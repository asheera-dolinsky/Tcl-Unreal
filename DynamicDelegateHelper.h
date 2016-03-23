/*
*	 The MIT License (MIT)
*
*	 Copyright (c) 2015 Alisa Dolinsky
*
*	 Permission is hereby granted, free of charge, to any person obtaining a copy
*	 of this software and associated documentation files (the "Software"), to deal
*	 in the Software without restriction, including without limitation the rights
*	 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*	 copies of the Software, and to permit persons to whom the Software is
*	 furnished to do so, subject to the following conditions:
*
*	 The above copyright notice and this permission notice shall be included in all
*	 copies or substantial portions of the Software.
*
*	 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*	 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*	 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*	 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*	 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*	 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*	 SOFTWARE.
*/

#pragma once

#include "Object.h"
#include "DynamicDelegateHelper.generated.h"


UCLASS()
class PHANTOMGUNSDEMO_API UDynamicDelegateHelper : public UObject
{
	GENERATED_BODY()
protected:
	UTclComponent* Interpreter = nullptr;
	FString Filename = "";
	FString Code = "";
public:
	void initialize(UTclComponent*, FString, FString);
	template<typename ...ParamTypes> void MAKE_DELEGATE(ParamTypes... params) {
		Interpreter->Fill(UTclComponent::pack(params...));
		Interpreter->Eval(Filename, Code);
		Interpreter->Purge();

	}
	template<typename RetDel, typename ...ParamTypes> static void Create(RetDel* del, UTclComponent* Interpreter, FString Filename, FString Code) {
		if(del != nullptr) {
			auto self = NewObject<UDynamicDelegateHelper>();
			self->initialize(Interpreter, Filename, Code);
			del->AddDynamic(self, &UDynamicDelegateHelper::MAKE_DELEGATE<ParamTypes...>);
		}
	}

};
