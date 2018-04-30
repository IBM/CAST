# encoding: utf-8
#================================================================================
#
#    multi-grok.rb
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
#================================================================================o

require "grok-pure"

# Reduce the number of file operations to load a collection of extraction patterns.
# 
# The default grok configuration does not play nicely with plugins which require a number of
# patterns to be compiled. This utility aggregates a collection of patterns and extraction 
# functions in an array for fast access.
#
#
class MultiGrok < Grok

    # A structure for holding the regexp and capture function of a pattern.
    CompiledPattern = Struct.new(:regexp, :cap_funct)

    # A collection of compiled patterns accessed in the Match and Exeuct functions.
    attr_accessor :compiled_patterns
    
    public
    def initialize
        super
        @compiled_patterns = Array.new()
    end #initialize

    public 
    def compile_pattern(pattern, named_captures_only=false)
        # Compile the pattern as normal.
        if compile(pattern, named_captures_only)
            @compiled_patterns << CompiledPattern.new(@regexp, @captures_func)
            return @compiled_patterns.length - 1
        else
            raise PatternError, "Unable to compile capture function for #{pattern}"
        end # compile
    end # compile_pattern
    
    def capture(pattern_idx, match, &block)
        compiled_pattern = @compiled_patterns[pattern_idx]
        unless compiled_pattern.nil?
           compiled_pattern.cap_funct.call(match, &block) 
        else
            return nil
        end #compiled_pattern.nil?
    end # capture

    def execute(pattern_idx, text)
        compiled_pattern = @compiled_patterns[pattern_idx]
        unless compiled_pattern.nil?
            compiled_pattern.regexp.match(text)
        else
            return false
        end #compiled_pattern.nil?
    end # execute

end # Multigrok
