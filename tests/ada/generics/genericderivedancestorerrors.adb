procedure GenericDerivedAncestorErrors is
    type Root is tagged null record;
    type I is interface;
    generic
        type T is new Root;
    package Missing_Private is
    end Missing_Private;
    generic
        type T is new Integer with private;
    package Untagged_Extension is
    end Untagged_Extension;
    generic
        type T is abstract new Integer;
    package Abstract_Scalar is
    end Abstract_Scalar;
    generic
        type T is limited new Root with private;
    package Limited_Ancestor is
    end Limited_Ancestor;
    generic
        type T is new I with private;
    package Interface_Ancestor is
    end Interface_Ancestor;
begin
    null;
end GenericDerivedAncestorErrors;
