set(src_dirs        ./)

set(include_dirs    ./)

set(requires        cat_detect)

if (IDF_TARGET STREQUAL "esp32s3")
    list(APPEND requires esp32_s3_eye_noglib
                         esp_lcd)
elseif (IDF_TARGET STREQUAL "esp32p4")
    list(APPEND requires esp32_p4_function_ev_board_noglib
                         esp_lcd)
endif()

set(embed_files     "cat.jpg")

idf_build_get_property(component_targets __COMPONENT_TARGETS)
if ("___idf_espressif__esp-dl" IN_LIST component_targets)
idf_component_get_property(espdl_dir espressif__esp-dl COMPONENT_DIR)
elseif("___idf_esp-dl" IN_LIST component_targets)
idf_component_get_property(espdl_dir esp-dl COMPONENT_DIR)
endif()
set(cmake_dir ${espdl_dir}/fbs_loader/cmake)
include(${cmake_dir}/utilities.cmake)
# For partition mode, don't embed the model file as binary data
# The model will be flashed directly to the cat_det partition
# list(APPEND embed_files ../models/cat_detect/models/s3/espdet_pico_224_224_cat.espdl)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires} EMBED_FILES ${embed_files})

# For partition mode, use esptool_py_flash_to_partition to flash the model to the partition
# target_add_aligned_binary_data(${COMPONENT_LIB} ${embed_files} BINARY)
if (IDF_TARGET STREQUAL "esp32s3")
    set(cat_model_file ${COMPONENT_DIR}/../models/cat_detect/models/s3/espdet_pico_224_224_cat.espdl)
elseif (IDF_TARGET STREQUAL "esp32p4")
    set(cat_model_file ${COMPONENT_DIR}/../models/cat_detect/models/p4/espdet_pico_224_224_cat.espdl)
endif()

esptool_py_flash_to_partition(flash "cat_det" "${cat_model_file}")
