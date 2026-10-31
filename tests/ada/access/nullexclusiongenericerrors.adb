procedure NullExclusionGenericErrors is
    type Link is access all Integer;
    procedure Nullable (Value : Link) is
    begin
        null;
    end Nullable;
    generic
        with procedure Action (Value : not null Link);
    procedure Apply;
    procedure Apply is
    begin
        null;
    end Apply;
    procedure Instance is new Apply (Nullable);
    function Null_Result return Link is
    begin
        return null;
    end Null_Result;
    generic
        with function Value return not null Link;
    procedure Read_Value;
    procedure Read_Value is
    begin
        null;
    end Read_Value;
    procedure Result_Instance is new Read_Value (Null_Result);
    subtype Required_Link is not null Link;
    generic
        with procedure Action (Value : Required_Link);
    package Outer is
        procedure Allowed is new Apply (Action);
    end Outer;
    package body Outer is
        procedure Rejected is new Apply (Action);
    end Outer;
begin
    null;
end NullExclusionGenericErrors;
