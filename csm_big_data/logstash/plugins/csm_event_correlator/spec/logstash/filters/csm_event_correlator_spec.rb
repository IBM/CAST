# encoding: utf-8
# ================================================================================
#
#  grok_dynamic_spec.rb
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
# ================================================================================
require "logstash/devutils/rspec/spec_helper"
require "logstash/filters/grok_dynamic"
require "logstash/codecs/plain"
require "logstash/event"

module LogStash::Environment
    # This module is configured using samples in the LogStash source code.
    # Set the logstash home for future spec components.
    unless self.const_defined?(:LOGSTASH_HOME)
        LOGSTASH_HOME = "/opt/Logstash"
    end

    unless self.method_defined?(:pattern_path)
        def pattern_path(path)
            ::File.join(LOGSTASH_HOME,"patterns",path)
        end
    end
end



describe LogStash::Filter::GrokDynamic do
    describe "mellanox system event log" do
        config <<-CONFIG
            filter{
                grok_dynamic {
                    dynamic_patterns_dir => [""]
                    data_field     => "event_description"
                    selector_field => "event_id"
                }

        CONFIG
        
        sample i"[Source 248a0703006d40f0_12  TO Dest: e41d2d0300ff4939_2]: Link went down: 248a0703006d40f0 (Switch: c931ibsw-leaf01) : 12 - e41d2d0300ff4938 (Computer: c931f02p18 HCA-1) : 2" do
            insist { subject.get("s_host") } == "c931ibsw-leaf01",
            insist { subject.get("s_port") } == "12",
            insist { subject.get("d_host") } == "c931f02p18",
            insist { subject.get("d_port") } == "2"
        end

    end

    describe "closing" do
        subject(:plugin) do
            ::LogStash::Filters::Grok.new()
        end

        before do 
            plugin.register
        end

        it "closed cleanly" do
            expect {plugin.do_close}.not_to raise_error
        end


    end
end
