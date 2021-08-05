# create library from source files
ADD_LIBRARY(dcmdata cmdlnarg dcbytstr dcchrstr dccodec dcdatset dcddirif dcdicdir dcdicent dcdict dcdictzz dcdirrec dcelem dcerror dcfilefo dchashdi dcistrma dcistrmb dcistrmf dcistrmz dcitem dclist dcmetinf dcobject dcostrma dcostrmb dcostrmf dcostrmz dcpcache dcpixel dcpixseq dcpxitem dcrleccd dcrlecce dcrlecp dcrledrg dcrleerg dcrlerp dcsequen dcstack dcswap dctag dctagkey dctypes dcuid dcwcache dcvm dcvr dcvrae dcvras dcvrat dcvrcs dcvrda dcvrds dcvrdt dcvrfd dcvrfl dcvris dcvrlo dcvrlt dcvrobow dcvrof dcvrpn dcvrpobw dcvrsh dcvrsl dcvrss dcvrst dcvrtm dcvrui dcvrul dcvrulup dcvrus dcvrut dcxfer dcpath vrscan vrscanl)

# declare installation files
INSTALL_TARGETS(${INSTALL_LIBDIR} dcmdata)

# Various dictionary-related commands
set(DICOMDIC   "${dcmdata_SOURCE_DIR}/data/dicom.dic")
set(PRIVATEDIC "${dcmdata_SOURCE_DIR}/data/private.dic")
set(VQDIC "${VQPATH}/extern/dicom-ns.dic")
set(DICTIONARIES "${DICOMDIC}" "${PRIVATEDIC}" "${VQDIC}")

TARGET_LINK_LIBRARIES(dcmdata ofstd oflog)
TARGET_LINK_LIBRARIES(dcmdata ${ZLIB_LIBS})

ADD_EXECUTABLE(mkdictbi EXCLUDE_FROM_ALL mkdictbi dctagkey dcdicent dcdict dcdictbi dcvr dchashdi cmdlnarg dctypes)
ADD_EXECUTABLE(mkdeftag EXCLUDE_FROM_ALL mkdeftag dctagkey dcdicent dcdict dcdictbi dcvr dchashdi cmdlnarg dctypes)
TARGET_LINK_LIBRARIES(mkdictbi ofstd oflog ${WIN32_STD_LIBRARIES})
TARGET_LINK_LIBRARIES(mkdeftag ofstd oflog ${WIN32_STD_LIBRARIES})

ADD_CUSTOM_TARGET(gendeftag
        COMMAND mkdeftag ${DICTIONARIES} > ${dcmdata_SOURCE_DIR}/include/dcmtk/dcmdata/dcdeftag.h
        DEPENDS mkdeftag
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generate a new include file of defined tags")

ADD_CUSTOM_TARGET(builtindict
        COMMAND mkdictbi ${DICTIONARIES} > ${dcmdata_SOURCE_DIR}/libsrc/dcdictbi.cc
        DEPENDS mkdictbi
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generate a builtin data dictionary")