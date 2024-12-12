/******************************************************************************************
*                                                                                         *
*    Test Node                                                                            *
*                                                                                         *
*    Copyright (c) 2023 Onur AKIN <https://github.com/onurae>                             *
*    Licensed under the MIT License.                                                      *
*                                                                                         *
******************************************************************************************/

#include "TestNode.hpp"

void TestNode::Build()
{
    auto a = CoreNodeInput("Input1", PortType::In, PortDataType::Float);
    auto b = CoreNodeInput("Input2", PortType::In, PortDataType::Double);
    auto c = CoreNodeInput("Input3", PortType::In, PortDataType::Int);
    auto d = CoreNodeInput("Input4", PortType::In, PortDataType::Double);
    auto e = CoreNodeOutput("Output1", PortType::Out, PortDataType::Int);
    auto f = CoreNodeOutput("Output2", PortType::Out, PortDataType::Double);
    auto g = CoreNodeOutput("Output3", PortType::Out, PortDataType::Generic);
    AddInput(a);
    AddInput(b);
    AddInput(c);
    AddInput(d);
    AddOutput(e);
    AddOutput(f);
    AddOutput(g);
    BuildGeometry();
}

void TestNode::DrawProperties(const std::vector<CoreNode*>& coreNodeVec)
{
    ImGui::Text(GetLibName().c_str());
    ImGui::Separator();
    ImGui::Text("This is a test module explanation.");
    ImGui::NewLine();
    ImGui::Text("Parameters");
    ImGui::Separator();
    EditName(coreNodeVec);
    parameter1.Draw(modifFlag);
    parameter2.Draw(modifFlag);
}
/*
void TestNode::SaveProperties(pugi::xml_node& xmlNode)
{
    SaveDouble(xmlNode, "parameter1", parameter1.Get());
    SaveDouble(xmlNode, "parameter2", parameter2.Get());
}

void TestNode::LoadProperties(pugi::xml_node& xmlNode)
{
    parameter1.Set(LoadDouble(xmlNode, "parameter1"));
    parameter2.Set(LoadDouble(xmlNode, "parameter2"));
}

*/
