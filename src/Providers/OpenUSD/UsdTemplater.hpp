#ifndef PROVIDERS_OPENUSD_TEMPLATER_HPP
#define PROVIDERS_OPENUSD_TEMPLATER_HPP

#include <pxr/usd/usd/stage.h>
#include <map>
#include <set>
#include <string>
#include <vector>

class UsdTemplater
{
    struct data;
    data* self = nullptr;

public:
    UsdTemplater();
    ~UsdTemplater();

    struct TemplateData {
        std::string rootDir;
        std::string shotName;
        PXR_NS::UsdMetadataValueMap commonMetaData;
        std::set<std::string> builtLayers;
        PXR_NS::UsdStageRefPtr stage;
        std::map<std::string, PXR_NS::VtValue> dict;
    };

    PXR_NS::UsdPrim GetTemplatePrim(const std::string& path);
    PXR_NS::UsdMetadataValueMap GetTemplateStageMetadata();

    void InstantiateTemplate(int recursionLevel,
                             TemplateData& templateData,
                             PXR_NS::UsdPrim templateRoot,
                             PXR_NS::UsdPrim templateAnchor);
}; // struct TemplateMinorData::data

#endif
