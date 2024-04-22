
algtype(getAlgorithmContextFromNative) vers(getAlgorithmContextFromNative);
algtype(initAlgorithmContext) vers(initAlgorithmContext);
algtype(processAlgorithmContext) vers(processAlgorithmContext);

static bool vers(getJNIfunctions)() {
	static	std::string_view alglib=jniAlglib;
	void *handle=openlib(alglib);
	if(!handle) {
		LOGGER("dlopen %s failed: %s\n",alglib.data(),dlerror());
		return false;
		}
{	constexpr const char str[]=algjavastr(getAlgorithmContextFromNative);
	vers(getAlgorithmContextFromNative)= (algtype(getAlgorithmContextFromNative)) dlsym(handle,str);
	 if(!vers(getAlgorithmContextFromNative)) {
	 	LOGGER("dlsym %s failed: %s\n",str,dlerror());
		return false;
	 	}
      }

{	constexpr const char str[]=algjavastr(initAlgorithmContext);
	vers(initAlgorithmContext)= (algtype(initAlgorithmContext)) dlsym(handle,str);
	 if(!vers(initAlgorithmContext)) {
	 	LOGGER("dlsym %s failed: %s\n",str,dlerror());
		return false;
	 	}
      }
{	constexpr const char str[]=algjavastr(processAlgorithmContext);
	vers(processAlgorithmContext)= (algtype(processAlgorithmContext)) dlsym(handle,str);
	 if(!vers(processAlgorithmContext)) {
	 	LOGGER("dlsym %s failed: %s\n",str,dlerror());
		return false;
	 	}
}

	typedef   jint (*OnLoadtype)(JavaVM* vm, void* reserved) ;
	constexpr const char onloadname[]="JNI_OnLoad";
	 OnLoadtype OnLoad= (OnLoadtype)dlsym(handle, onloadname);
	 if(!OnLoad) {
		LOGGER("dlsym %s failed\n",onloadname);
		return false;
		}
	LOGSTRING("found OnLoad\n");

	OnLoad(getnewvm(),nullptr);
	LOGAR("after OnLoad");
	return true;
	}

double AlgorithmContext::vers(process)(int index,double value, double temp) {
   return processAlgorithmContext(subenv,nullptr,reinterpret_cast<jobject>(this),index,value,temp,0.0,4.4,11.1);
};

AlgorithmContext *vers(initAlgorithm)(SensorGlucoseData *sens) {
    jobject jalg= vers(getAlgorithmContextFromNative)(subenv,nullptr);
    char *shortname=sens->getinfo()->siBlueToothNum;
    int res = vers(initAlgorithmContext)(subenv,nullptr,jalg, 0, reinterpret_cast<jstring>(shortname));
    if(res != 1) {
        LOGGER("initAlgorithmContext(algcontext,0,%s)==%d\n",shortname,res);
        return nullptr;
    }
    auto algcontext=reinterpret_cast<AlgorithmContext *>(jalg);
     loadjson(sens, sens->vers(statefile).data(),algcontext); 
     return algcontext;
     }
