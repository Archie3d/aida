with GenericClasswideModel;
procedure GenericClasswideErrors is
    package Model renames GenericClasswideModel;
    package Other is
        type Root is tagged null record;
        type Flag is interface;
        procedure Ping (Object : Flag) is abstract;
        type Has_Flag is new Model.Root and Flag with null record;
        overriding procedure Ping (Object : Has_Flag) is null;
    end Other;
    generic
        type T is new Model.Root with private;
    package Definite_Formal is
    end Definite_Formal;
    generic
        type T (<>) is new Model.Root with private;
    package Indefinite_Formal is
    end Indefinite_Formal;
    generic
        type T (<>) is new Model.Root and Other.Flag with private;
    package Progenitor_Formal is
    end Progenitor_Formal;
    package Good is new Progenitor_Formal (Other.Has_Flag'Class);
    package Wrong_Progenitor is new Progenitor_Formal (Model.Child'Class);
    package Wrong_Ancestor is new Indefinite_Formal (Other.Root'Class);
    package Not_Definite is new Definite_Formal (Model.Root'Class);
    generic
        type T (<>) is new Model.Root with private;
    package Bad_Access is
    end Bad_Access;
    package body Bad_Access is
        function Score (Object : T) return Integer is
            type Callback is access function (Object : T) return Integer;
            Call : Callback := Read'Access;
        begin
            return Call (Object);
        end Score;
    end Bad_Access;
    generic
        type T (<>) is new Model.Root with private;
    package Bad_Ancestor is
        generic
            type U (<>) is new T'Class with private;
        package Inner is
        end Inner;
    end Bad_Ancestor;
begin
    null;
end GenericClasswideErrors;
