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

    struct TemplateEvaluationContext {
        std::string rootDir;
        std::string shotName;
        PXR_NS::UsdMetadataValueMap commonMetaData;
        std::set<std::string> builtLayers;
        PXR_NS::UsdStageRefPtr templateStage;
        std::map<std::string, PXR_NS::VtValue> dict;
    };

    PXR_NS::UsdPrim GetTemplatePrim(const std::string& path);
    PXR_NS::UsdMetadataValueMap GetTemplateStageMetadata();
    PXR_NS::UsdStageRefPtr GetTemplateStage();

    void InstantiateTemplate(int recursionLevel,
                             TemplateEvaluationContext& templateData,
                             PXR_NS::UsdPrim templateRoot,
                             PXR_NS::UsdPrim templateAnchor);
}; // struct TemplateMinorData::data

#endif
