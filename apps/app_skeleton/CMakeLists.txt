# app_skeleton — a self-registering app component.
# WHOLE_ARCHIVE keeps the registration constructor from being dropped by the linker
# (nothing references this component's symbols directly; it registers itself at boot).
idf_component_register(
    SRCS "app_skeleton.c"
    REQUIRES taskmaster_core
    WHOLE_ARCHIVE
)
