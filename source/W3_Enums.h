#ifndef W3_ENUMS_H
#define W3_ENUMS_H

#define GENERATE_ENUM_STRINGS

#ifndef GENERATE_ENUM_STRINGS
    #define DECL_ENUM( element ) element
    #define BEGIN_ENUM( ENUM_NAME ) typedef enum tag##ENUM_NAME
    #define END_ENUM( ENUM_NAME ) ENUM_NAME; \
            char* getString##ENUM_NAME(enum tag##ENUM_NAME index);
#else
    #define DECL_ENUM( element ) #element
    #define BEGIN_ENUM( ENUM_NAME ) char* gs_##ENUM_NAME [] =
    #define END_ENUM( ENUM_NAME ) ; char* getString##ENUM_NAME(enum \
            tag##ENUM_NAME index){ return gs_##ENUM_NAME [index]; }
#endif

BEGIN_ENUM(EAnimEffectAction)
{
    DECL_ENUM(EA_Start),
    DECL_ENUM(EA_Stop)
} END_ENUM(EAnimEffectAction)

BEGIN_ENUM(EItemAction)
{
    DECL_ENUM(IA_Mount),
    DECL_ENUM(IA_MountToHand),
    DECL_ENUM(IA_MountToLeftHand),
    DECL_ENUM(IA_MountToRightHand),
    DECL_ENUM(IA_Unmount)
} END_ENUM(EAnimEffectAction)

BEGIN_ENUM(EItemEffectAction)
{
    DECL_ENUM(IEA_Start),
    DECL_ENUM(IEA_Stop)
} END_ENUM(EAnimEffectAction)

BEGIN_ENUM(EDropAction)
{
    DECL_ENUM(DA_DropRightHand),
    DECL_ENUM(DA_DropLeftHand),
    DECL_ENUM(DA_DropAny)
} END_ENUM(EAnimEffectAction)

BEGIN_ENUM(EItemLatentAction)
{
    DECL_ENUM(ILA_Draw),
    DECL_ENUM(ILA_Holster),
    DECL_ENUM(ILA_Switch)
} END_ENUM(EAnimEffectAction)

#endif // W3_ENUMS_H
