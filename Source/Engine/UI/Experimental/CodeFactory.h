
//[Nori_SC] u can call me lazy :D
#if CodeFactory

#define DecFieldGetSet(Type,Name)   \
API_PROPERTY() void Set##Name(Type In##Name);\
API_PROPERTY() Type Get##Name() const;       \

#define API_PROPERTY(...)

#define ImpSetGet(Type,Name,SeterCode,Other)\
void UIComponent::Set##Name(Type In##Name)  \
{                                           \
Other.Set##Name(In##Name);  SeterCode       \
}                                           \
Type UIComponent::Get##Name() const         \
{                                           \
 return Other.Get##Name();                  \
}
#error "the CodeFactory is enabled, disable it and compile again"
#endif // CodeFactory
