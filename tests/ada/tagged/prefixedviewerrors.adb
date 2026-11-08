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
    function Bound return Integer renames X.Read;
    type BoundReader is access function return Integer;
    Callback : BoundReader := Bound'Access;
    procedure WrongMode (Amount : in out Integer) renames X.Add;
    NotAliased : Prefixed_Model.Root;
    procedure NeedsAliased (Value : Integer) renames NotAliased.Set;
    type Roots is array (1 .. 2) of Prefixed_Model.Root;
    function AggregateComponent return Integer renames
        Roots'(others => (Value => 1))(1).Read;
    function MakeRoots return Roots is
    begin
        return (others => (Value => 1));
    end MakeRoots;
    function TemporaryComponent return Integer renames MakeRoots (1).Read;
    package Source is
        function Make return Roots renames MakeRoots;
    end Source;
    function QualifiedTemporary return Integer renames Source.Make (1).Read;
begin
    null;
end PrefixedViewErrors;
