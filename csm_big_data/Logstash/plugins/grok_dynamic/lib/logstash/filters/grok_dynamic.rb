# encoding: utf-8
#================================================================================
#
# grok_dynamic.rb
#
#  © Copyright IBM Corporation 2015,2016. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

require "logstash/filters/base"
require "logstash/filters/grok"
require "logstash/namespace"


#
# 
class LogStash::Filters::GrokDynamic < LogStash::Filters::Grok
    # Name of the configuration block in Logstash.
    config_name "grok_dynamic"
    
    config :data_field, :validate => :string, :default => ""
    #
    config :selector_field, :validate => :string, :default => ""

    config :dynamic_patterns_dir, :validate => :array, :default => []

    public
    def register
        @patterns_dir += @dynamic_patterns_dir
        @match = {@data_field => "" }

        if @patterns_files_glob.nil?
            @patterns_files_glob = "*"
        end
        #
        # Load the defaults, then we compile all of the patterns in the supplied file.
        super
        
        # TODO Check to see if  [0] will ever fail. jdunham@us.ibm.com
        require "logstash/filters/grok_dynamic_pure"
        @dynamic_grok = GrokDynamic.new
        @dynamic_grok.copy_grok(@patterns[@data_field][0])
        @patterns[@data_field][0] = @dynamic_grok

        # Open the patterns to find the ones we care about. 
        pattern_names=[]
        paths = @dynamic_patterns_dir
    
        #  Open the dynamic paths.
        paths.each do |path|
            if File.directory?(path)
                path = File.join(path, @patterns_files_glob)
            end 

            # Adpated from Grok
            Dir.glob(path).each do |file_name|
                pattern_names += get_pattern_names(file_name)
            end
        end
        
        @dynamic_grok.precompile(pattern_names,@named_captures_only)
    end # def register

    private 
    def get_pattern_names(path)
        pattern_names = []

        file = File.new(path,"r")

        file.each do |line|
            next if line =~ /^\s*#/
            name, pattern = line.gsub(/^\s*/, "").split(/\s+/, 2)
            pattern_names << name  
        end

        return pattern_names
    ensure
        file.close
    end # def get_pattern_names

    public
    def filter(event)
        selector      = event[@selector_field]
        if selector
            if @dynamic_grok.set_regexp(selector)
                #@logger.info? and @logger.info(:pattern_name => @dynamic_grok.set_regexp(selector))
                #@logger.info? and @logger.info(:grok => @patterns[@data_field][0])
                super
            end
        end
    end # def event
    
end # class LogStash::Filters::GrokDynamic
