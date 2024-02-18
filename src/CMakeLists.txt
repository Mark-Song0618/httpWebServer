add_executable(webServer)

# add src files
file(GLOB src
		  *.cpp
		  utils/*.cpp
	)

file(GLOB headers
		  *.h
		  utils/*.h	 
	)

target_sources(webServer PRIVATE ${src})

# mysql
set(MYSQL_ROOT "C:/Program Files/MySQL/MySQL Server 8.0")
target_include_directories(webServer PRIVATE ${MYSQL_ROOT}/include)
target_link_directories(webServer PRIVATE 
								  ${MYSQL_ROOT}/lib
								  ${MYSQL_ROOT}/bin)
target_link_libraries(webServer libmysql mysqlclient)
