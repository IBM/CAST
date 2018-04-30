package ClustConf;

#================================================================================
#   
#    hcdiag/src/tests/common/ClusterConf.pm
# 
#  © Copyright IBM Corporation 2015,2016. All Rightgpu_functionss Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#=============================================================================

# common test options.

use strict;
use warnings;
use File::Basename;
use File::Spec;
use Data::Dumper;
use Class::Struct;
use Getopt::Long;
use YAML;



sub new        # constructor
{
   my $class = shift; 
   my $self = {config=>{}};     
   bless ($self, $class) ; 
   #$self->{num} = $number;
   return $self;        
}


# load the configuration, 
sub load {
   my $self=shift;
   my ($cfgFile) = @_;  # optional paramter. if not defined, load the default config file.
   if ((! defined $cfgFile) || ($cfgFile eq "")) {
      $cfgFile = dirname(__FILE__) . "/../../etc/clustconf.yaml";
   }
   my $fh;
   if (! open ($fh, '<', $cfgFile )) {
     die "can't open config file: $cfgFile $!\n";
   }
   my $config = YAML::LoadFile($fh);
   close($fh);

   if (! defined $config) {
      die "can't parse yaml file: $cfgFile $!\n";
   }
   $self->{config} = $config;
   return 1;
}

sub getConfig {
   my $self=shift;
   return($self->{config});
}
#
# find regex match to an arbitraty list with an arbitary 
#   regex field.
# regex
#   list -- array to search for match in.
#   field -- name with regex to search in.
#   value -- value to search for
# information.
sub findRecInDb {
   my $self = shift;
   my ($list,$field,$value) = @_;
   if (!defined $value) { 
      return(undef);
   }
   for my $c (@$list) {
      my $f = $c->{$field};
      next if (!$f);
      #print 'case='.Dumper($f);
      my $rx=$f;
      if ($value =~ m/$rx/) {
         return($c);
      }
   }
   return(undef);
} 

# locate the node name in the configuration and return its configuration 
# record.  
# information.
sub findNodeCfg {
   my $self = shift;
   my ($node) = @_;
   if (!defined $node) { 
      return(undef);
   }
   my $nodeList=$self->{config}->{node_info};
   for my $c (@$nodeList) {
      next if (!defined $c->{case});
      #print 'case='.Dumper($c->{case});
      my $rx=$c->{case};
      if ($node =~ m/$rx/) {
         return($c);
      }
   }
   return(undef);
} 



# locate the rvitals associated with this node....
sub findRvitals {
   my $self = shift;
   my ($node) = @_;
   my $ncfg = $self->findNodeCfg($node);
   if (! defined $ncfg) {
      return(undef);
      # should we return error tuple??
      # "$node: Clustconf->findNodeCfg($node) failed");
   }
   # no rvitals section for this node... skip
   my $rvtag=$ncfg->{rvitals};
   next if (! defined $rvtag);
   my $rvitals=$self->{config}->{rvitals}->{$rvtag};
   if (! defined $rvitals) {
      return(undef);
      # should we return error tuple??
      # "vitals: {$rvtag} not found in yaml file\n");
   }
   return($rvitals);
}

# scan the rvitals list for the id field and return the first match...
# param: rvitals -- rvitials returned by findRvitals
#        id -- id to search for.
sub findRvitalById {
   my $self = shift;
   my ($rvitals, $id) = @_;

   for my $rv (@$rvitals) {
      my $rx = "^".$rv->{id}.'$';;
      if ($id =~ /$rx/) {
         return($rv);
      }
   }
   return(undef);
}



# obligatory initialztion function...
1;
