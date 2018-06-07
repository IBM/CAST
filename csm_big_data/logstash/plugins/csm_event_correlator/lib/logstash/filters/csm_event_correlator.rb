# encoding: utf-8
#================================================================================
#
#    csm_event_correlator.rb
#
#    © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

# Author : John Dunham (jdunham@us.ibm.com)

require "logstash/filters/base"
require "logstash/namespace"
require "logstash/environment"
require "logstash/patterns/core"
require "json"
require "yaml"
require "uri"


# Correlate structured events 
#
#
class LogStash::Filters::CSMEventCorrelator < LogStash::Filters::Base
    # Name of the configuration block in Logstash.
    config_name "csm_event_correlator"
    
    # Specifies a yaml file 
    config :events_dir, :validate => :string, :default => "/etc/logstash/conf.d/events.yml"
    
    config :named_captures_only, :validate => :boolean, :default => true

    config :patterns_dir, :validate => :array, :default => []

    @@default_patterns = [LogStash::Patterns::Core.path, LogStash::Environment.pattern_path("*")]

    # A collection of pre cooked actions, for use in the 
    @@default_actions = { send_ras: "post_ras(ras_msg_id, ras_location, ras_timestamp, raw_data);" }
    
    public
    def register
        # Attempt to load the yaml map
        @ras_map=YAML.safe_load(File.open(@events_dir))                    
        # TODO walk the YAML to make sure it was properly configured.

        ras_create_url= @ras_map["ras_create_url"] 
        csm_target    = @ras_map["csm_target"]
        csm_port      = @ras_map["csm_port"]

        unless ras_create_url.nil? || csm_target.nil? || csm_port.nil?
            @csm_uri = URI.parse("http://#{csm_target}:#{csm_port}#{ras_create_url}")
        else
            @logger.warn("URI could not be constructed! Verify 'ras_create_url', 'csm_target' and 'csm_port' were configured in #{csm_ras_yml}")
        end 

        # Build the grok engine.
        require "logstash/filters/multi-grok-pure"
        @multi_grok = MultiGrok.new()    
        @multi_grok.logger  = @logger unless @logger.nil? 
        add_patterns_from_paths(@@default_patterns, @multi_grok, "*")
        add_patterns_from_paths(@patterns_dir    , @multi_grok, "*")

        unless @ras_map.has_key?("data_sources") 
            @logger.warn("data_sources not specified")
        end

        # Iterate over the data sources to verify they've all bee properly constructed, building any patterns.
        @ras_map["data_sources"].each do | key, value | 
            # Verify the keys were set.
            if  value.has_key?("ras_location") && value.has_key?("ras_timestamp") &&
                value.has_key?("event_data") && value.has_key?("category_key") &&
                value.has_key?("categories")
                
                # Iterate over the categories.
                value["categories"].each do | cat, cat_value |
                    cat_value.each do | pattern | 


                        # Verify the category has been configured
                        # TODO break.
                        if pattern.has_key?("pattern") 
                            # Compile the key pattern.
                            pattern[:pattern_idx] = @multi_grok.compile_pattern(pattern["pattern"], @named_captures_only)
                        end # category test
                        
                        pattern[:action] = process_action_string ( pattern["action"] )
                        pattern[:extract] = pattern.has_key?("extract") && pattern["extract"]
                    end
                    #value["categories"][cat] = cat_value
                end # category iteration.
                #@ras_map["data_sources"][key] = value
            end # data_source test
        end # ras_map iteration
        

    end # def register
    
    protected

    # Construct the lambda function for the process action.
    def process_action_string(action_str)
        action = (action_str.gsub(/%{([^}]*)}/, "event.get(\"\\1\")").
            gsub(/\.([a-z_]*);/, "%{\\1}"))
        action = action % @@default_actions
        eval "lambda { | event, temp_event, ras_msg_id, ras_location, ras_timestamp, raw_data | #{action} }"
    end # def process_action

    # Posts a RAS event and moves on. 
    def post_ras(message_id, location, timestamp, raw_data, retry_attempt=0)
        # TODO move to debug.
        @logger.info("Posting ras message: #{message_id}; #{timestamp}; #{location}; #{raw_data}") 
        header= {'Content-Type': 'text/json'}

        ras_event = { 
            msg_id:        message_id,
            location_name: location,
            time_stamp:    timestamp,
            raw_data:      raw_data
        }

        http = Net::HTTP.new(@csm_uri.host, @csm_uri.port);
        request = Net::HTTP::Post.new(@csm_uri.request_uri, header)
        request.body = ras_event.to_json

        begin
            response = http.request(request)
        rescue Timeout::Error, Errno::EINVAL, Errno::ECONNRESET, Errno::ECONNREFUSED, EOFError,
               Net::HTTPBadResponse, Net::HTTPHeaderSyntaxError, Net::ProtocolError => e
            logger.warn("Unable send RAS event to: #{@csm_uri}\nMessage ID: #{message_id}\n#{e.message}")
            #
            # TODO set tag for RAS generation failure.
        #ensure 
        #    request.finsh() 
        end
    end #def post RAS

    public
    def filter(event)
        type_key    = event.get("type")
        data_source = @ras_map["data_sources"][type_key]

        unless data_source.nil?
            cat_key     = data_source["category_key"]
            raw_data    = data_source["event_data"]
            patterns    = data_source["categories"].fetch(event.get(cat_key),[])

            patterns.each do |pattern|
                grok_idx = pattern[:pattern_idx]
                match = grok_timeout(grok_idx, event.get(raw_data))

                if match
                    temp_event = pattern[:extract]  ? event : event.clone()

                    @multi_grok.capture(grok_idx,match) { 
                        |attribute,value| process(attribute, value, temp_event) }
                    

                    # Build the message id with the event.
                    pattern[:action].call(
                        temp_event,
                        temp_event.sprintf(pattern["ras_msg_id"]),
                        temp_event.get(data_source["ras_location"] ), 
                        temp_event.get(data_source["ras_timestamp"]), 
                        temp_event.get(raw_data) )

                    break # Exit loop
                end # match
            end # category
        end # data_source
    end # def filter(event)

    def grok_timeout(grok_idx, value)
        return @multi_grok.execute(grok_idx, value)
    end # grok_timeout

    def process(attr, value, event)
        unless value.nil?
            event.set(attr, value) 
        end # value.nil
    end # process

    def add_patterns_from_paths(paths, grok, glob)
        # This pattern loader is adapted from the official grok filter.
        # Since this implementation only has one grok, we don't need to persist pattern files.
        paths.each do | path | 
            if File.directory?(path)
                path = File.join(path, glob)
            end # File.directory


            Dir.glob(path).each do |file|
                unless File.directory?(file) 
                    if File.exists?(file)
                        grok.add_patterns_from_file(file)
                    else
                        raise "File does not exist: #{file}"
                    end # File.exists
                end # File.directory
            end # Dir.glob
        end # paths.each
    end # add_patterns_from_paths

    def close
    end # close
    
end # class LogStash::Filters::CSMEventCorrelator
