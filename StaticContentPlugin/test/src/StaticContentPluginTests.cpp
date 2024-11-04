/**
 * @file StaticContentPluginTests.cpp
 * 
 * This module contains unit tests of the 
 * StaticContentPlugin class.
 * 
 * © 2024 by Hatem Nabli
 */

#include <functional>
#include <gtest/gtest.h>
#include <WebServer/PluginEntryPoint.hpp>
#include <SystemUtils/File.hpp>

#ifdef _WIN32
#define API __declspec(dllimport)
#else /* POSIX */
#define API
#endif /* _WIN32 / POSIX */
extern "C" API void LoadPlugin(
    Http::IServer* server,
    Json::Json configuration,
    SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate diagnosticMessagedelegate,
    std::function< void() >& unloadDelegate
);

struct MockServer
    : public Http::IServer
{
    // Properties
    /**
     * This is the resource subspace path that the unit under
     * test has registered.
     */
    std::vector< std::string > registeredResourcesSubspacePath;
    /**
     * This is the delegate that the unit under test has registered
     * to be called to handel resource requests.
     */
    ResourceDelegate registredResourceDelegate;

    // Methods

    // IServer
public:
    virtual std::string GetConfigurationItem(const std::string& key) override {
        return "";
    }
    virtual void SetConfigurationItem(const std::string& key, const std::string& value) override {
        return;
    }
    virtual SystemUtils::DiagnosticsSender::UnsubscribeDelegate SubscribeToDiagnostics(
        SystemUtils::DiagnosticsSender::DiagnosticMessageDelegate delegate,
        size_t minLevel = 0
    ) override {
        return [](){};
    }
    virtual UnregistrationDelegate RegisterResource(
        const std::vector< std::string >& resourceSubspacePath, 
        ResourceDelegate resourceDelegate
    ) override {
        registeredResourcesSubspacePath = resourceSubspacePath;
        registredResourceDelegate = resourceDelegate;
        return [](){};
    }
};

struct StaticContentPluginTests
    : public ::testing::Test
{
    // Properties

    /**
     * This is the temporary directory to use to test
     * the file class
     */
    std::string testAreaPath;

    // Methods

    // ::testing::Test

    virtual void SetUp() {
        testAreaPath = SystemUtils::File::GetExeParentDirectory() + "/TestArea";
        ASSERT_TRUE(SystemUtils::File::CreateDirectory(testAreaPath));
    }

    virtual void TearDown() {
        ASSERT_TRUE(SystemUtils::File::DeleteDirectory(testAreaPath));
    }
};


TEST_F(StaticContentPluginTests, StaticContentPluginTest_Load_Test) {
    MockServer server;
    SystemUtils::File testFile(testAreaPath + "/exemple.txt");
    (void)testFile.OpenReadWrite();
    testFile.Write("Hello", 5);
    (void)testFile.Close();
    std::function< void() > unloadDelegate;
    Json::Json configuration(Json::Json::Type::Object);
    configuration.Set("space", "/");
    configuration.Set("root",testAreaPath );
    LoadPlugin(
        &server,
        configuration,
        [](
            std::string senderName,
            size_t level,
            std::string message
        ){
            printf(
                "[%s:%zu] %s\n",
                senderName.c_str(),
                level,
                message.c_str()
            );
        },
        unloadDelegate
    );
    const auto request = std::make_shared< Http::IServer::Request >();
    request->target.SetPath({"exemple.txt"});
    const auto response = server.registredResourceDelegate(request);
    ASSERT_EQ("Hello", response->body);
    ASSERT_FALSE(unloadDelegate == nullptr);
}

TEST_F(StaticContentPluginTests, StaticContentPluginTest_checkforEtag_Test) {
    MockServer server;
    SystemUtils::File testFile(testAreaPath + "/exemple.txt");
    (void)testFile.OpenReadWrite();
    testFile.Write("Hello", 5);
    (void)testFile.Close();
    std::function< void() > unloadDelegate;
    Json::Json configuration(Json::Json::Type::Object);
    configuration.Set("space", "/");
    configuration.Set("root",testAreaPath );
    LoadPlugin(
        &server,
        configuration,
        [](
            std::string senderName,
            size_t level,
            std::string message
        ){
            printf(
                "[%s:%zu] %s\n",
                senderName.c_str(),
                level,
                message.c_str()
            );
        },
        unloadDelegate
    );
    // Send initila request to get the entity tag 
    // of the test file.
    auto request = std::make_shared< Http::IServer::Request >();
    request->target.SetPath({"exemple.txt"});
    auto response = server.registredResourceDelegate(request);
    ASSERT_EQ(200, response->statusCode);
    ASSERT_TRUE(response->headers.HasHeader("ETag"));
    const auto etag = response->headers.GetHeaderValue("ETag");

    // Send second conditional request for the test file
    // this time expecting to 304 Not Modified response.
    request = std::make_shared<Http::IServer::Request>();
    request->target.SetPath({"exemple.txt"});
    request->headers.SetHeader("If-None-Match", etag);
    response = server.registredResourceDelegate(request);
    EXPECT_EQ(304 , response->statusCode);
    EXPECT_EQ("Not Modified" , response->status);
    EXPECT_TRUE(response->body.empty());
}