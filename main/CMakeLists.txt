# Firmware entry component — a trivial stub that starts the sealed OS
# (taskmaster_run()). Apps come from main/idf_component.yml (generated from apps.yaml);
# the core is pulled as the taskmaster_core git dependency. Do not edit.
idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES taskmaster_core
)
