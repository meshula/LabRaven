#ifndef AUDIOCONTEXT_MANAGER_HPP
#define AUDIOCONTEXT_MANAGER_HPP

class AudioContextManager {
    struct data;
    data* _self;
public:
    AudioContextManager();
    ~AudioContextManager();
    void RunUI();
};

#endif