procedure GenericNullContractErrors is
    package Model is
        type Root is tagged null record;
        procedure Hook (Object : Root) is null;
    end Model;
    generic
        type T is new Model.Root with private;
        with procedure Hook (Object : T) is null;
    procedure Illegal (Object : T'Class);
    procedure Illegal (Object : T'Class) is
    begin
        Hook (Object);
    end Illegal;
begin
    null;
end GenericNullContractErrors;
