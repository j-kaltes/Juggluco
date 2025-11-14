#pragma once
#include <span>
#include <string_view>
#include <string.h>
struct Agent_data {
    int labelsize;
    int descriptionsize;
    char side;
    char label[];
    public:
 static   Agent_data* newAgent(char side,const std::string_view label,const std::span<const char> descrip) {
        int labelsize= label.size();
        int datalen =labelsize+1+descrip.size()+1+sizeof(Agent_data);
        Agent_data *agent=reinterpret_cast<Agent_data *>(new(std::align_val_t(alignof(Agent_data)),std::nothrow) char[datalen]);
        agent->labelsize=labelsize;
        agent->descriptionsize=descrip.size();
        agent->labelsize=labelsize;
        agent->side=side;
        memcpy(agent->label,label.data(),labelsize);
        agent->label[labelsize]='\0';
        memcpy(agent->label+labelsize+1, descrip.data(),descrip.size());
        agent->label[labelsize+1+descrip.size()]='\0';
        return agent;
        }
 static void deleteAgent(Agent_data *agent) {
    ::operator delete[] (reinterpret_cast<char*>(agent),std::align_val_t(alignof(Agent_data)));
    }
    int getWhere() const {
        return side!='0';
        }
    std::string_view getLabel() const {
        return {label,size_t(labelsize)};
        }
    std::span<const char> getDescription() const {
        return std::span(label+labelsize+1,size_t(descriptionsize));
        }
    int datalen() const {
        return labelsize+1+descriptionsize+1+sizeof(Agent_data);
        }
    };
