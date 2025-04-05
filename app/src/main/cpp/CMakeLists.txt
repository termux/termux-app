# won't this help more than mk?
project("termux-bootstrap")
add_library(${PROJECT_NAME} SHARED termux-bootstrap.c)
