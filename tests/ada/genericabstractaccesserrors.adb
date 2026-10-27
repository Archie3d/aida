with GenericAbstractModel;
procedure GenericAbstractAccessErrors is
    generic
        with function Read (Object : GenericAbstractModel.Child; Extra : Integer) return Integer is abstract;
    package Views is
    end Views;
    package body Views is
        type Callback is access function (Object : GenericAbstractModel.Child; Extra : Integer) return Integer;
        Call : Callback := Read'Access;
    end Views;
begin
    null;
end GenericAbstractAccessErrors;
