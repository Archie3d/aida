with GenericAbstractModel;
procedure GenericAbstractErrors is
    package Model renames GenericAbstractModel;
    type Other is tagged null record;
    type Named_Access is access all Model.Root;
    generic
        with procedure Missing is abstract;
    package No_Control is
    end No_Control;
    generic
        with function Scalar (Object : Integer) return Integer is abstract;
    package Scalar_Control is
    end Scalar_Control;
    generic
        with procedure Wide (Object : Model.Root'Class) is abstract;
    package Wide_Control is
    end Wide_Control;
    generic
        with procedure Named (Object : Named_Access) is abstract;
    package Named_Control is
    end Named_Control;
    generic
        with function Multiple (Object : Model.Root) return Other is abstract;
    package Two_Controls is
    end Two_Controls;
    function Ordinary (Object : Model.Root; Extra : Integer := 5) return Integer is
    begin
        return Extra;
    end Ordinary;
    generic
        type T is abstract tagged private;
        with function Score (Object : T; Extra : Integer := 5) return Integer is abstract;
    package Views is
    end Views;
    package Good is new Views (Model.Root, Model.Read);
    package Bad is new Views (Model.Root, Ordinary);
    generic
        with function Score (Object : Model.Root; Extra : Integer := 5) return Integer is abstract Ordinary;
    package Bad_Default is
    end Bad_Default;
    generic
        with function Score (Object : Model.Root; Extra : Integer := 5) return Integer is abstract Model.Read;
    package Good_Default is
    end Good_Default;
    generic
        with procedure Change (Object : Model.Child; Amount : Integer) is abstract;
    package Modes is
    end Modes;
    package Wrong_Mode is new Modes (Model.Change);
    generic
        with function Plain (Object : Model.Root; Extra : Integer) return Integer;
    package Concrete_Formal is
    end Concrete_Formal;
    package Abstract_Actual is new Concrete_Formal (Model.Read);
    generic
        type T is tagged private;
        with procedure Update (Object : in out T) is abstract;
    package Outer is
        package Wrong is new Concrete_Formal (Model.Read);
    end Outer;
    package Null_Model is
        type Root is tagged null record;
        function Read (Object : Root; Pointer : Named_Access) return Integer;
    end Null_Model;
    package body Null_Model is
        function Read (Object : Root; Pointer : Named_Access) return Integer is
        begin
            return 0;
        end Read;
    end Null_Model;
    generic
        with function Read (Object : Null_Model.Root; Pointer : not null Named_Access)
            return Integer is abstract Null_Model.Read;
    package Bad_Null_Default is
    end Bad_Null_Default;
begin
    null;
end GenericAbstractErrors;
