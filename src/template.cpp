#include "template.h"
#include <string>

void Template::TemplateRenderer::addTemplate(const string &name, const string &content) {
    mustache::mustache tmpl(content);
    this->cache.insert(std::pair<std::string, mustache::mustache>(name, tmpl));
}

string Template::TemplateRenderer::addTemplateFile(const string &filename, const string &name) {
    std::filesystem::path path(filename);
    std::string templateName = (name.empty()) ? path.filename().stem().string() : name;
    auto content = File::read(path.string());
    this->addTemplate(templateName, content);
    return templateName;
}

std::string Template::TemplateRenderer::render(const string &templateName, mustache::data &data) {
    auto templateIterator = this->cache.find(templateName);
    if (templateIterator == this->cache.end()) {
        throw runtime_error("Template not found: " + templateName);
    }
    return templateIterator->second.render(data);
}

mustache::data Template::dataFromJson(json &json) {
    mustache::data result;
    if (json.is_string()) {
        return json.template get<string>();
    }
    if (json.is_number()) {
        if (json.is_number_float()) {
            auto num = json.template get<float>();
            return to_string(num);
        }
        auto num = json.template get<int>();
        return to_string(num);
    }
    if (json.is_array()) {
        mustache::data array{mustache::data::type::list};
        for (auto & item : json) {
            array.push_back(dataFromJson(item));
        }
        return array;
    }
    for (auto& [key, value] : json.items()) {
        result.set(key, dataFromJson(value));
    }
    return result;
}

mustache::data Template::dataFromJson(const string &json) {
    auto parsed = json::parse(json);
    return dataFromJson(parsed);
}
