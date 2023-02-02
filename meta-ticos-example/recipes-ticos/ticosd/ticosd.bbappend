do_install:append() {
    echo "{" > ${D}${sysconfdir}/ticosd.conf
    echo "  \"base_url\": \"${TICOS_BASE_URL}\"," >> ${D}${sysconfdir}/ticosd.conf
    echo "  \"project_key\": \"${TICOS_PROJECT_KEY}\"," >> ${D}${sysconfdir}/ticosd.conf
    echo "  \"software_type\": \"${TICOS_SOFTWARE_TYPE}\"," >> ${D}${sysconfdir}/ticosd.conf
    echo "  \"software_version\": \"${TICOS_SOFTWARE_VERSION}\"" >> ${D}${sysconfdir}/ticosd.conf
    echo "}" >> ${D}${sysconfdir}/ticosd.conf
}
