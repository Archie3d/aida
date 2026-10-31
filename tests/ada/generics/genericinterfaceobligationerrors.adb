procedure GenericInterfaceObligationErrors is
    package Model is
        type With_Operation is interface;
        procedure Required_Operation (X : With_Operation) is abstract;
        type Empty is interface;
    end Model;
    generic
        type F is interface;
    package Builder is
        type Concrete is new F with null record;
    end Builder;
    package Bad is new Builder (Model.With_Operation);
    package Good is new Builder (Model.Empty);
begin
    null;
end GenericInterfaceObligationErrors;
