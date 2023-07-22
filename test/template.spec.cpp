#include "gtest/gtest.h"

#include <filesystem>

#include "../src/template.h"

#include "../src/json.hpp"

using namespace std;
using namespace nlohmann;

namespace {

    mustache::data fromJson(json & json) {
        mustache::data result;
        if (json.is_number() ||json.is_string()) {
            return json.template get<string>();
        }
        if (json.is_array()) {
            mustache::data array{mustache::data::type::list};
            for (auto & item : json) {
                array.push_back(fromJson(item));
            }
            return array;
        }
        for (auto& [key, value] : json.items()) {
            result.set(key, fromJson(value));
        }
        return result;
    }

    TEST(Templates, RenderTest) {
        const char TEST_TEMPLATE[] = R"__(<html><h1>{{title}}</h1></p>{{content}}</p></html>)__";

        mustache::mustache temp(TEST_TEMPLATE);
        mustache::data templateData;
        templateData.set("title", "Foobar");
        templateData.set("content", "asdf 1234");
        auto result = temp.render(templateData);

        ASSERT_STREQ(result.c_str(), "<html><h1>Foobar</h1></p>asdf 1234</p></html>");

        Template::TemplateRenderer renderer;
        renderer.addTemplate("test", TEST_TEMPLATE);

        auto renderResult = renderer.render("test", templateData);

        ASSERT_STREQ(renderResult.c_str(), "<html><h1>Foobar</h1></p>asdf 1234</p></html>");

        // Render file
        auto tmpPath = filesystem::temp_directory_path();
        tmpPath /= "test_template.mustache";
        File::write(tmpPath.string(), TEST_TEMPLATE);

        renderer.addTemplateFile(tmpPath.string());

        auto renderFileResult = renderer.render("test_template", templateData);

        ASSERT_STREQ(renderFileResult.c_str(), "<html><h1>Foobar</h1></p>asdf 1234</p></html>");

        filesystem::remove(tmpPath);
    }

    TEST(Templates, Json_to_Template_Data) {
        const char TEST_TEMPLATE[] = R"__(<html><h1>{{title}}</h1></p>{{content}}</p><ul>{{#list}}<li>{{.}}</li>{{/list}}</ul></html>)__";
        const char TEST_JSON[] = R"json(
        { "title": "FOO BAR JSON", "content": "Han shot first", "list": [ "Foo", "Bar", "ASDF" ], "obj": { "foo": "bar", "bar": "baz" } }
        )json";

        auto json = json::parse(TEST_JSON);
        auto data = fromJson(json);

        Template::TemplateRenderer renderer;
        renderer.addTemplate("test", TEST_TEMPLATE);
        auto result = renderer.render("test", data);
        cout << result << endl;
    }
}

