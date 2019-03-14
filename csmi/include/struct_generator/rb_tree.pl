#!/bin/perl 
#================================================================================
#
#    csmi/include/rb_tree.pl
#
#  © Copyright IBM Corporation 2015-2017. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
#
#================================================================================

use strict;
use warnings;
use Scalar::Util 'refaddr';

# This is an extremely basic implementation of a red black tree to generate the constant array
# based trees used in the CSM API code.

sub InsertRBNode {
    my ($tree, $hash, $data) = @_;

    my $prev_node = undef;
    my $curr_node = $tree;

    # Find the appropriate leaf first.
    while ( defined $curr_node  )
    {
        $prev_node = $curr_node;

        if   ( $hash < $curr_node->{HASH} ) { $curr_node = $curr_node->{LEFT}; }
        elsif( $hash > $curr_node->{HASH} ) { $curr_node = $curr_node->{RIGHT}; }
        else 
        { 
            print "COLLISON: THIS RB IMPLEMENTATION DOESN'T SUPPORT COLLISION! $data\n";
            #$curr_node->{DATA} =~ s/\@NULL@/$data/i;  
            return; 
        } # EARLY RETURN ON COLLISION
    }
    
    $curr_node = {};
    $curr_node->{DEPTH}     = 0;
    $curr_node->{HASH}      = $hash;
    $curr_node->{DATA}      = $data;
    $curr_node->{PARENT}    = $prev_node;
    $curr_node->{LEFT}      = undef;
    $curr_node->{RIGHT}     = undef;
    $curr_node->{RED}       = 1;
    
    if ( ! defined $prev_node )
    {
        $_[0]=$curr_node;
    }
    elsif ( $hash < $prev_node->{HASH} )
    {
        $prev_node->{LEFT} = $curr_node;
    }
    else
    {
        $prev_node->{RIGHT} = $curr_node;
    }

    my $curr_target = $curr_node;
    my $sibling     = undef;

    while ( defined $curr_target->{PARENT} && $curr_target->{PARENT}->{RED} )
    {
        if ( defined $curr_target->{PARENT}->{PARENT}->{LEFT} and
            $curr_target->{PARENT} == $curr_target->{PARENT}->{PARENT}->{LEFT} )
        {
            my $sibling = $curr_target->{PARENT}->{PARENT}->{RIGHT};
            if ( defined $sibling && $sibling->{RED} )
            {
                 $curr_target->{PARENT}->{RED} = 0;
                 $sibling->{RED} = 0;
                 $sibling->{PARENT}->{RED} = 1;
                 $curr_target = $curr_target->{PARENT}->{PARENT};
            }
            else
            {
                if ( defined $curr_target->{PARENT}->{RIGHT} and
                    $curr_target == $curr_target->{PARENT}->{RIGHT} )
                {
                    $curr_target = $curr_target->{PARENT};
                    LeftRBRotate($_[0], $curr_target);
                }
                $curr_target->{PARENT}->{RED} = 0;
                $curr_target->{PARENT}->{PARENT}->{RED} = 1;
                RightRBRotate($_[0], $curr_target->{PARENT}->{PARENT});
            }
        }
        else
        {
            my $sibling = $curr_target->{PARENT}->{PARENT}->{LEFT};
            if ( defined $sibling && $sibling->{RED} )
            {
                 $curr_target->{PARENT}->{RED} = 0;
                 $sibling->{RED} = 0;
                 $sibling->{PARENT}->{RED} = 1;
                 $curr_target = $curr_target->{PARENT}->{PARENT};
            }
            else
            {
                if ( ref($curr_target->{PARENT}->{LEFT}) and
                    $curr_target == $curr_target->{PARENT}->{LEFT} )
                {
                    $curr_target = $curr_target->{PARENT};
                    RightRBRotate($_[0], $curr_target);
                }
                $curr_target->{PARENT}->{RED} = 0;
                $curr_target->{PARENT}->{PARENT}->{RED} = 1;
                LeftRBRotate($_[0], $curr_target->{PARENT}->{PARENT});
            }
        }
    }

    $_[0]->{RED} = 0;
}

sub LeftRBRotate{
    my ($root, $node) = @_;

    my $y = $node->{RIGHT};
    $node->{RIGHT} = $y->{LEFT};

    if ( defined $y->{LEFT} )
    {
        $y->{LEFT}->{PARENT} = $node;
    }
    $y->{PARENT} = $node->{PARENT};

    if( defined $node->{PARENT} ) # Rotated node has A parent.
    {
        if ( defined $node->{PARENT}->{LEFT} and
                refaddr($node) == refaddr($node->{PARENT}->{LEFT}) ) # Rotated node is a left child.
        {
            $node->{PARENT}->{LEFT} = $y;
        }
        else
        {
            $node->{PARENT}->{RIGHT} = $y;
        }
    }
    else # Rotated node is a right child.
    {
        $_[0] = $y;
    }
    $y->{LEFT} = $node;
    $node->{PARENT} = $y;
}

sub RightRBRotate{
    my ($root, $node) = @_;
    
    my $x = $node->{LEFT};
    $node->{LEFT} = $x->{RIGHT};
    
    if ( defined $x->{RIGHT} )
    {
        $x->{RIGHT}->{PARENT} = $node;
    }
    
    $x->{PARENT} = $node->{PARENT};
    
    if( defined $node->{PARENT} ) # Rotated node A parent.
    {
        if ( defined $node->{PARENT}->{RIGHT} and
            refaddr($node) == refaddr($node->{PARENT}->{RIGHT}) ) # Rotated node is a right child.

        {
            $node->{PARENT}->{RIGHT} = $x;
        }
        else # Rotated node is a left  child.
        {
            $node->{PARENT}->{LEFT} = $x;
        }
    }
    else
    {
        $_[0] = $x;
    }
    $x->{RIGHT} = $node;
    $node->{PARENT} = $x;
}

# @brief Generates a csv with each entry equating to the data entry of a node on the tree.
# @param[in] $0 - The tree to traverse.
# @param[in] $1 - The default string, used for empty nodes.
#
# @return (field_count, array_string) - The field count is the number of entries in the array string.
#                                     - The array string generated by the function call.
#
sub RBArrayOrder{
    my ($tree,$default_string) = @_;
    my $string = "{"; # The output string.
    my $null_string =""; # A repeated null string 

    my @tree_queue = ($tree); # Initialize the queue with the root node.

    my $num_fields  = 0; # The number of fields added to the tree.
    my $null_count  = 0; # The current counting of empty tree nodes.
    
    my $fields_per_row = 1;
    my $total_field_count = $fields_per_row;
    my $depth = 0;
    print("\n");

    my $terminal_node = {};
    $terminal_node->{DEPTH} = -1;

    if ( defined $tree )
    {
        $tree->{DEPTH} = 0;
    }

    # A queue is used to gereate an array mapping of the tree for the C const.
    while (@tree_queue)
    {
        my $node = shift @tree_queue;
        if (defined $node )
        {
            if ($node->{DEPTH} == -1 )
            {
                $null_string.= "$default_string,\n"; 
                $null_count++;
                next;
            }

            # If a real value was found after a string of nulls add it to the string.
            if ($null_count > 0)
            {
                $string     .= $null_string;
                $null_string = "";
                $num_fields += $null_count;
                $null_count = 0;
            }

            if( $node->{DEPTH}  > $depth )
            {
                
                my $null_fill_count = $total_field_count - $num_fields;
                #print "$node->{DATA} $node->{DEPTH} $depth $null_fill_count\n";
                for ( my $i=0; $i < $null_fill_count; $i++ )
                {
                    $string .= "$default_string,\n";
                    $num_fields++;
                }
                $depth++;
                $fields_per_row = 2**$depth;
                $total_field_count += $fields_per_row;
               # $string .="\n";
            }

            $string .= "$node->{DATA},\n"; # Add the data string to the print out.

            # Verify the left tree is not null.
            if ( defined  $node->{LEFT} ) { $node->{LEFT}->{DEPTH} = $depth+1; push @tree_queue, $node->{LEFT}; }
            else { push @tree_queue, undef; }

            # Verify the right tree is not null.
            if ( defined  $node->{RIGHT} ) { $node->{RIGHT}->{DEPTH} = $depth+1;push @tree_queue, $node->{RIGHT}; }
            else { push @tree_queue,undef; }
            
            $num_fields++; # Increase the counter for valid entries.
        }
        else
        {
            push @tree_queue,$terminal_node;
            push @tree_queue,$terminal_node;
            $null_string.= "$default_string,\n"; 
            $null_count++;
        }

        $fields_per_row--;
        if ( $fields_per_row == 0 )
        {
            #$string     .= $null_string;
            #$null_string = "";
            #$num_fields += $null_count;
            #$null_count = 0;
            $depth++;
            $fields_per_row = 2**$depth;
            $total_field_count += $fields_per_row;
        #    $string.="\n";
        }
    }

    substr($string,-2)="";
    $string .="}\n";
   
    return ($num_fields,$string);
}
1; #true value for compilation
