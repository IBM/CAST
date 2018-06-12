#================================================================================
#   
#    buckets/basic/hc_diag.sh
# 
#  © Copyright IBM Corporation 2015-2018. All Rights Reserved
#
#    This program is licensed under the terms of the Eclipse Public License
#    v1.0 as published by the Eclipse Foundation and available at
#    http://www.eclipse.org/legal/epl-v10.html
#
#    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
#    restricted by GSA ADP Schedule Contract with IBM Corp.
# 
#================================================================================

# Bucket for hcdiag testing
# run through hcdiag tests

# Try to source the configuration file to get global configuration variables
if [ -f "${BASH_SOURCE%/*}/../../csm_test.cfg" ]
then
        . "${BASH_SOURCE%/*}/../../csm_test.cfg"
else
        echo "Could not find csm_test.cfg file expected at "${BASH_SOURCE%/*}/../csm_test.cfg", exitting."
        exit 1
fi

# Local Variables
LOG=${LOG_PATH}/buckets/basic/hcdiag.log
HC_DIAG_PATH=/opt/ibm/csm/hcdiag

echo "------------------------------------------------------------" >> ${LOG}
echo "                 Starting hcdiag Bucket" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
date >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}

echo "CALLING hcdiag test ppping..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test ppping --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_ppping.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\nppping FAILED"
fi

echo "CALLING hcdiag test test_memsize..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test test_memsize --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_test_memsize.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\ntest_memsize FAILED"
fi

echo "CALLING hcdiag test test_simple..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test test_simple --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_test_simple.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\ntest_simple FAILED"
fi

echo "CALLING hcdiag hxecache..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test hxecache --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_hxecache.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\nhxecache FAILED"
fi

echo "CALLING hcdiag hxecpu..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test hxecpu --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_hxecpu.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\nhxecpu FAILED"
fi

echo "CALLING hcdiag hxefpu64..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test hxefpu64 --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_hxefpu64.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\nhxefpu64 FAILED"
fi

echo "CALLING hcdiag hxemem64..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test hxemem64 --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_hxemem64.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\nhxemem64 FAILED"
fi

echo "CALLING hcdiag daxpy..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test daxpy --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_daxpy.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\ndaxpy FAILED"
fi

echo "CALLING hcdiag dcgm_diag..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test dcgm-diag --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_dcgm_diag.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\ndcgm_diag FAILED"
fi

echo "CALLING hcdiag dgemm..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test dgemm --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_dgemm.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\ndgemm FAILED"
fi

echo "CALLING hcdiag jlink..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test jlink --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_jlink.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\njlink FAILED"
fi

echo "CALLING hcdiag nvvs..." >> ${LOG}
${HC_DIAG_PATH}/bin/hcdiag_run.py --test nvvs --target ${COMPUTE_NODES} > ${LOG_PATH}/test/hcdiag_nvvs.log 2>&1
RC=$?
if [ ${RC} -eq 0 ]
then
        echo "PASS" >> ${LOG}
else
        echo "FAILED" >> ${LOG}
        FLAGS+="\nnvvs FAILED"
fi

echo "------------------------------------------------------------" >> ${LOG}
echo "                hcdiag Bucket COMPLETED" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
echo "Additional Flags:" >> ${LOG}
echo -e "${FLAGS}" >> ${LOG}
echo "------------------------------------------------------------" >> ${LOG}
exit 0
