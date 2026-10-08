#pragma once
#include "Processor.h"
namespace blazar {
class UserPatches {
 Processor& processor;
public:
 juce::File root;
 static juce::File defaultDirectory(){
  auto directory=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
 #if JUCE_MAC
  // JUCE returns ~/Library on macOS; application files belong in Application Support.
  directory=directory.getChildFile("Application Support");
 #endif
  return directory.getChildFile("Dog Lab Plugins/Blazar/User Patches");
 }
 explicit UserPatches(Processor& p,juce::File location=defaultDirectory()):processor(p),root(location){}
 static bool validName(const juce::String& name){
  if(name.isEmpty()||name.length()>80||name!=name.trim()||name.endsWithChar('.')||name.containsAnyOf("/\\:*?\"<>|")||name.containsChar(0))return false;
  for(auto c:name)if(c<32)return false;
  auto stem=name.upToFirstOccurrenceOf(".",false,false).toUpperCase();
  if(stem=="CON"||stem=="PRN"||stem=="AUX"||stem=="NUL"||stem=="FACTORY")return false;
  for(int i=1;i<=9;++i)if(stem=="COM"+juce::String(i)||stem=="LPT"+juce::String(i))return false;
  return true;
 }
 juce::StringArray banks()const{juce::StringArray a;for(auto f:root.findChildFiles(juce::File::findDirectories,false))if(validName(f.getFileName()))a.add(f.getFileName());a.sort(true);return a;}
 juce::StringArray patches(const juce::String& bank)const{juce::StringArray a;if(!validName(bank))return a;for(auto f:root.getChildFile(bank).findChildFiles(juce::File::findFiles,false,"*.blazar.json"))a.add(f.getFileName().dropLastCharacters(12));a.sort(true);return a;}
 juce::Result createBank(const juce::String& bank){if(!validName(bank))return juce::Result::fail("Use a non-empty bank name without filesystem punctuation. Factory is reserved.");return root.getChildFile(bank).createDirectory();}
 juce::File file(const juce::String& bank,const juce::String& name)const{return root.getChildFile(bank).getChildFile(name+".blazar.json");}
 juce::Result save(const juce::String& bank,const juce::String& name,bool overwrite){
  if(!validName(bank)||!validName(name))return juce::Result::fail("Invalid bank or patch name.");
  auto result=createBank(bank);if(result.failed())return result;
  auto target=file(bank,name);if(target.existsAsFile()&&!overwrite)return juce::Result::fail("A patch with that name already exists. Select it and use Save, or choose another name.");
  auto* values=new juce::DynamicObject;
  for(auto* parameter:processor.getParameters())if(auto* identified=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter))values->setProperty(identified->paramID,parameter->getValue());
  auto* object=new juce::DynamicObject;object->setProperty("format","Blazar user patch");object->setProperty("version",3);object->setProperty("name",name);object->setProperty("parameters",juce::var(values));
  juce::TemporaryFile temporary(target);
  if(!temporary.getFile().replaceWithText(juce::JSON::toString(juce::var(object),false))||!temporary.overwriteTargetFileWithTemporary())return juce::Result::fail("Could not write patch. Check folder permissions.");
  return juce::Result::ok();
 }
 juce::Result load(const juce::String& bank,const juce::String& name){
  if(!validName(bank)||!validName(name))return juce::Result::fail("Invalid patch path.");
  auto object=juce::JSON::parse(file(bank,name));auto* values=object["parameters"].getDynamicObject();
  if(object["format"].toString()!="Blazar user patch"||(int(object["version"])!=1&&int(object["version"])!=2&&int(object["version"])!=3)||!values)return juce::Result::fail("Not a supported Blazar patch.");
  const char* split[]={"fieldStructure","fieldTone","fieldAttack","fieldRelease"};const char* old[]={"structure","tone","attack","release"};
  for(int i=0;i<4;++i)if(!values->hasProperty(split[i]))values->setProperty(split[i],values->getProperty(old[i]));
  const char* plasmaDetails[]={"plasmaRate","plasmaCoherence","plasmaIntermittency","plasmaResonance"};
  for(auto id:plasmaDetails)if(!values->hasProperty(id))values->setProperty(id,.5);
  if(int(object["version"])==1&&values->hasProperty("plasmaModel"))values->setProperty("plasmaModel",double(values->getProperty("plasmaModel"))*2./11.);
  if(int(object["version"])<3&&values->hasProperty("pulseRate")){static constexpr double rates[]={.25,.5,1,2,3,4};double x=double(values->getProperty("pulseRate"));if(std::isfinite(x)&&x>=0&&x<=1)values->setProperty("pulseRate",std::log2(rates[std::clamp(int(std::round(x*5)),0,5)]/.25)/4.);}
  // Validate the entire recipe before changing any parameter.
  for(auto* parameter:processor.getParameters())if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter)){
   auto value=values->getProperty(id->paramID);double x=double(value);
   if(!(value.isDouble()||value.isInt()||value.isInt64())||!std::isfinite(x)||x<0||x>1)return juce::Result::fail("Missing or invalid parameter: "+id->paramID);
  }
  for(auto* parameter:processor.getParameters())if(auto* id=dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter)){
   parameter->beginChangeGesture();parameter->setValueNotifyingHost(float(values->getProperty(id->paramID)));parameter->endChangeGesture();
  }
  return juce::Result::ok();
 }
 juce::Result removeBank(const juce::String& bank){
  if(!validName(bank))return juce::Result::fail("Factory cannot be deleted; invalid user bank name.");
  auto target=root.getChildFile(bank);
  if(target.getParentDirectory()!=root||target.isSymbolicLink()||!target.isDirectory())return juce::Result::fail("Not a normal user bank folder.");
  return target.deleteRecursively()?juce::Result::ok():juce::Result::fail("Could not delete bank. Check folder permissions.");
 }
 juce::Result remove(const juce::String& bank,const juce::String& name){if(!validName(bank)||!validName(name))return juce::Result::fail("Invalid patch path.");return file(bank,name).deleteFile()?juce::Result::ok():juce::Result::fail("Could not delete patch.");}
};
}
