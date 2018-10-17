# Detect clang version, switch to homebrew llvm automatically
if(APPLE)
	SET(USE_LLVM_FROM_BREW NO)
	if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.1)
		message(STATUS "clang ${CMAKE_CXX_COMPILER_VERSION} does not support C++17.")
		SET(USE_LLVM_FROM_BREW YES)
	elseif(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 10.1)
		message(STATUS "clang ${CMAKE_CXX_COMPILER_VERSION} does not ship the proper libc++.")
		SET(USE_LLVM_FROM_BREW YES)
	endif()

	if(USE_LLVM_FROM_BREW)
		if(EXISTS "/usr/local/opt/llvm/bin/clang++")
			set(CMAKE_C_COMPILER /usr/local/opt/llvm/bin/clang CACHE PATH "" FORCE)
			set(CMAKE_CXX_COMPILER /usr/local/opt/llvm/bin/clang++ CACHE PATH "" FORCE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/opt/llvm/include -I/usr/local/opt/llvm/include/c++/v1/")
			set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -L/usr/local/opt/llvm/lib -Wl,-rpath,/usr/local/opt/llvm/lib")
			message(STATUS "Auto-detected brew-installed llvm version")
		else()
			message(FATAL_ERROR "Did not find a proper compiler. Please run \"brew install llvm\" or upgrade to XCode >= 10.1.")
		endif()
	endif()
endif()