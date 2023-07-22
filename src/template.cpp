#include "template.h"

void Template::TemplateRenderer::addTemplate(const string &name, const string &content) {
    mustache::mustache tmpl(content);
    this->cache.insert(std::pair<std::string, mustache::mustache>(name, tmpl));
}

void Template::TemplateRenderer::addTemplateFile(const string &filename, const string &name) {
    std::filesystem::path path(filename);
    std::string templateName = (name.empty()) ? path.filename().stem().string() : name;
    auto content = File::read(path.string());
    this->addTemplate(templateName, content);
}

std::string Template::TemplateRenderer::render(const string &templateName, mustache::data &data) {
    auto templateIterator = cache.find(templateName);
    if (templateIterator == this->cache.end()) {
        throw runtime_error("Template not found: " + templateName);
    }
    return templateIterator->second.render(data);
}
