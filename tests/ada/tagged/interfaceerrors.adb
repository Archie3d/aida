procedure InterfaceErrors is
    package Model is
        type Root is interface;
        function Value (Self : Root) return Integer is abstract;
        procedure Concrete (Self : in out Root);
        type With_Discriminant (D : Integer) is interface;
        type Plain is tagged null record;
        type Invalid_Progenitor is interface and Plain;
        type Wide_Progenitor is interface and Root'Class;
        type Duplicate is interface and Root and Root;
        type Scalar is new Integer and Root;
        type Missing is new Plain and Root with null record;
        type Complete is new Root with null record;
        overriding function Value (Self : Complete) return Integer;
        type Limited_Root is limited interface;
        type Limited_Item is limited new Limited_Root with null record;
        type Hidden is new Plain and Root with private;
        overriding function Value (Self : Hidden) return Integer;
    private
        type Hidden is new Plain with null record;
    end Model;
    package body Model is
        procedure Concrete (Self : in out Root) is
        begin
            null;
        end Concrete;
        overriding function Value (Self : Complete) return Integer is
        begin
            return 1;
        end Value;
        overriding function Value (Self : Hidden) return Integer is
        begin
            return 2;
        end Value;
    end Model;
    Abstract_Object : Model.Root;
    A, B : Model.Limited_Item;
begin
    A := B;
end InterfaceErrors;
