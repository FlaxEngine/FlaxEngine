
//[Nori_SC] u can call me lazy :D
#if CodeFactory

#define DecFieldGetSet(Type,Name)\
void Set##Name(Type In##Name);\
Type Get##Name() const;       \


#define ImpSetGet(Type,Name,SeterCode,Other)\
void UIComponent::Set##Name(Type In##Name)  \
{                                           \
Other##SeterCode                            \
}                                           \
Type UIComponent::Get##Name() const         \
{                                           \
 return Other##Name;                        \
}
#error "the CodeFactory is enabled, disable it and compile again"
#endif // CodeFactory
