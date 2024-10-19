#ifndef __CALLBACK_H__
#define __CALLBACK_H__


template<typename R, typename P>
struct FCallback
{
	typedef R(*FnCallback)(void* Param, P);

	void Init(FnCallback n_cb, void* n_Param)
	{
		Cb = n_cb;
		Param = n_Param;
	}

	R Execute(P p)
	{
		if (Cb) return Cb(Param, p);

		return R();
	}

protected:
	FnCallback Cb = nullptr;
	void* Param = nullptr;
};


#endif //!__CALLBACK_H__
