procedure NullExclusionErrors is
    type Link is access all Integer;
    subtype Required_Link is not null Link;
    subtype Bad_Scalar is not null Integer;
    subtype Repeated is not null Required_Link;
    procedure Action (Value : not null Link);
    procedure Action (Value : Link) is
    begin
        null;
    end Action;
    procedure Anonymous (Value : not null access Integer);
    procedure Anonymous (Value : access Integer) is
    begin
        null;
    end Anonymous;
    procedure Nullable (Value : Link) is
    begin
        null;
    end Nullable;
    P : Link;
    R : not null Link renames P;
    procedure Alias (Value : not null Link) renames Nullable;
    type Callback is access procedure (Value : not null Link);
    B : Callback := Nullable'Access;
    function Required_Result return not null Link;
    function Required_Result return Link is
    begin
        return null;
    end Required_Result;
    package Dispatching is
        type Root is tagged null record;
        procedure Control (Value : access Root);
    end Dispatching;
    package body Dispatching is
        procedure Control (Value : not null access Root) is
        begin
            null;
        end Control;
    end Dispatching;
begin
    null;
end NullExclusionErrors;
