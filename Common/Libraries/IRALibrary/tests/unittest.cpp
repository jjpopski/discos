#include "gtest/gtest.h"

#include "IRATools_test.i"
#include "FastQueue_test.i"
#include "Socket_test.i"


using namespace IRALibraryTest;

TEST_F(IRALibrary_IRATools, makeDirectory_createSimpleDirectory){
	EXPECT_TRUE(makeDirectory_createSimpleDirectory());
}

TEST_F(IRALibrary_IRATools, makeDirectory_createComplexDirectory){
	EXPECT_TRUE(makeDirectory_createComplexDirectory());
}

TEST_F(IRALibrary_IRATools, fileExists_checkExistance){
	EXPECT_TRUE(fileExists_checkExistance());
}

TEST_F(IRALibrary_IRATools, fileExists_checkNoExistance){
	EXPECT_TRUE(fileExists_checkNoExistance());
}

TEST_F(IRALibrary_IRATools, createEmptyFile_checkSuccess){
	EXPECT_TRUE(createEmptyFile_checkSuccess());
}

TEST_F(IRALibrary_IRATools, createEmptyFile_checkFail){
	EXPECT_TRUE(createEmptyFile_checkFail());
}

TEST_F(IRALibrary_IRATools, deleteFile_checkSuccess){
	EXPECT_TRUE(deleteFile_checkSuccess());
}

TEST_F(IRALibrary_IRATools,extractFileName_checkSimpleDirectoryPath){
	EXPECT_TRUE(extractFileName_checkSimpleDirectoryPath());
}

TEST_F(IRALibrary_IRATools,extractFileName_checkSimpleFilePath){
	EXPECT_TRUE(extractFileName_checkSimpleFilePath());
}

TEST_F(IRALibrary_IRATools, skyFrequency_noIntersection){
	EXPECT_TRUE(skyFrequency_noIntersection());
}

TEST_F(IRALibrary_IRATools,skyFrequency_intersection){
	EXPECT_TRUE(skyFrequency_intersection());
}

TEST_F(IRALibrary_FastQueue,FastQueue_checkLimits){
	EXPECT_TRUE(FastQueue_checkLimits());
}

TEST_F(IRALibrary_FastQueue,FastQueue_checkConsistency){
	EXPECT_TRUE(FastQueue_checkConsistency());
}

TEST_F(IRALibrary_Socket, sendWithoutConnection){
	EXPECT_TRUE(sendWithoutConnection());
}
