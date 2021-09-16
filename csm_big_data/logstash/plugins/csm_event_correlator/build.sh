#================================================================================
#
#    csm_big_data/logstash/plugins/csm_event_correlator/build.sh
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

ZIP_NAME="logstash-filter-csm-event-correlator.zip"
SCRIPT_HOME=$(pwd)

rm -rf logstash
mkdir -p logstash/dependencies

# Build the gem
rm -rf *.gem *.zip

# Build and move the gemfile.
gem build logstash-filter-csm_event_correlator.gemspec
mv logstash-filter-csm-event-correlator-*.gem logstash/

# Fetch all of the dependencies.
cd logstash/dependencies
gem fetch logstash-codec-plain -v 3.0.6
gem install jar-dependencies -v 0.4.0

# Return home and zip the file.
cd ${SCRIPT_HOME}
zip -r ${ZIP_NAME} logstash

rm -rf logstash

#if [ -f /usr/share/logstash/bin/logstash-plugin ]
#then
#    # Install the plugin
#    /usr/share/logstash/bin/logstash-plugin remove logstash-filter-csm-event-correlator
#    /usr/share/logstash/bin/logstash-plugin install logstash-filter-csm-event-correlator-*.gem
#
#    # Build the zip file.
#    /usr/share/logstash/bin/logstash-plugin prepare-offline-pack --output ${ZIP_NAME} \
#        logstash-filter-csm-event-correlator
#fi

