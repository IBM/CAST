#================================================================================
#
#    hcdiag/samples/gpu-health/CMakeLists.txt
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

set(HCDIAG_SAMPLE gpu-health)

file(GLOB SAMPLE_PROGRAM
  "makefile"
)

file(GLOB SAMPLE_FILE
  "gpu-health.cu"
)


install(PROGRAMS ${SAMPLE_PROGRAM} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/samples/${HCDIAG_SAMPLE})
install(FILES ${SAMPLE_FILE} COMPONENT ${HCDIAG_RPM_NAME} DESTINATION csm/hcdiag/samples/${HCDIAG_SAMPLE})

