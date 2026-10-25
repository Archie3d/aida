procedure InterfacePrivacyErrors is
    package Contracts is
        type API is interface;
        function Read (Self : API) return Integer is abstract;
    end Contracts;
    package Models is
        type Hidden is private;
        function Make return Hidden;
    private
        type Hidden is new Contracts.API with null record;
        overriding function Read (Self : Hidden) return Integer;
    end Models;
    package body Models is
        function Make return Hidden is
        begin
            return (Contracts.API with null record);
        end Make;
        overriding function Read (Self : Hidden) return Integer is
        begin
            return 1;
        end Read;
    end Models;
    Object : aliased Models.Hidden := Models.Make;
    View : Contracts.API'Class := Object;
    type Link is access all Contracts.API'Class;
    Reference : Link := Object'Access;
    Cast : Models.Hidden := Models.Hidden (View);
begin
    null;
end InterfacePrivacyErrors;
