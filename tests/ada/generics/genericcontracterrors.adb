with GenericClasswideModel;
procedure GenericContractErrors is
    package Model renames GenericClasswideModel;
    generic
        type T is private;
        type U is private;
    package Duplicate is
        type Tag is tagged null record;
        procedure P (Object : Tag; X : T);
        procedure P (Object : Tag; X : U);
    end Duplicate;
    package body Duplicate is
        procedure P (Object : Tag; X : T) is null;
        procedure P (Object : Tag; X : U) is null;
    end Duplicate;
    package Bad_Visible is new Duplicate (Integer, Integer);
    generic
        type T (<>) is tagged private;
        with function Measure (X : T) return Integer is abstract;
    package Abstract_View is end Abstract_View;
    function Read_Wide (X : Model.Root'Class) return Integer is
    begin
        return Model.Read (X);
    end Read_Wide;
    package Bad_Direct is new Abstract_View (Model.Root'Class, Read_Wide);
    generic
        type T (<>) is new Model.Root with private;
    package Bad_Template is
        subtype Double_Class is T'Class'Class;
        subtype Wide is T'Class;
        subtype Twice is Wide'Class;
    end Bad_Template;
    generic
        type T is private;
    function Invalid (Left, Right : T) return T;
    function Invalid (Left, Right : T) return T is
    begin
        return Left + Right;
    end Invalid;
    generic
        type T is limited private;
        X : T;
        Copy : T := X;
    package Invalid_Default is end Invalid_Default;
    type Pointer is access Integer;
    subtype NN is not null Pointer;
    generic
        Object : in out NN;
    package Private_Exclusion is
    private
        Alias_Value : not null Pointer renames Object;
    end Private_Exclusion;
    Nullable : Pointer;
    package Bad_Private is new Private_Exclusion (Nullable);
begin
    null;
end GenericContractErrors;
