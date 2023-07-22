#pragma once

#include <map>
#include <string>
#include <filesystem>
#include "mustache.hpp"
#include "file.h"

using namespace std;


namespace Template {
    class TemplateRenderer {
    private:
        std::map<std::string, mustache::mustache> cache;
    public:
        TemplateRenderer() = default;

        void addTemplate(const std::string & name, const std::string & content);

        void addTemplateFile(const std::string & filename, const std::string & name = "");

        std::string render(const std::string & templateName, mustache::data & data);
    };
}