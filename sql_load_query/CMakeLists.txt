add_library(resources STATIC loader.cpp metadb.cpp table.cpp query.cpp rowdata.cpp )

add_library(eidos::resources ALIAS resources)