with Prefixed_Model;
procedure PrefixedViewErrors is
    X : aliased Prefixed_Model.Root;
    type Reader is access function (Object : Prefixed_Model.Root) return Integer;
    F : Reader := X.Read'Access;
    function Alias (Object : Prefixed_Model.Root) return Integer renames X.Read;
    generic
        with function Read (Object : Prefixed_Model.Root) return Integer;
    procedure Generic_Read;
    procedure Generic_Read is
    begin
        null;
    end Generic_Read;
    procedure Instance is new Generic_Read (X.Read);
begin
    null;
end PrefixedViewErrors;
