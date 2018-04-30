# encoding: utf-8
#================================================================================
#
# grok_dynamic_pure.rb
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
require "grok-pure" 

class GrokDynamic < Grok

    # The compiled regular expressions
    attr_accessor :regexps

    public
    def initialize
        super
        @regexps = {}
    end # def initialize
    
    public
    def copy_grok( orig_grok )
        @patterns = orig_grok.patterns
    end # def copy_grok

    public
    def precompile(good_patterns, named_captures_only=false)
        @regexps = {}

        good_patterns.each do |pattern|
            if @patterns[pattern]
                compile(@patterns[pattern],named_captures_only)
                @regexps[pattern] = @regexp 
            end
        end
    end # def precompile

    public
    def set_regexp(pattern_name)
        hash = {:pattern => pattern_name, :regexps =>@regexps, :regexp => @regexp }
        if @regexps[pattern_name]    

            @regexp = @regexps[pattern_name]
            return true
        end

        return false
    end # def set_regexp
end # Grok
