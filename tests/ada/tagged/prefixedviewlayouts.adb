with Prefixed_Model;
with Prefixed_Layout_Library;
procedure PrefixedViewLayouts is
    Calls : Integer := 0;
    IndexCalls : Integer := 0;
    function Index return Integer is
    begin
        IndexCalls := IndexCalls + 1;
        return 2;
    end Index;
    type Roots is array (1 .. 2) of Prefixed_Model.Root;
    function MakeRoots return Roots is
    begin
        Calls := Calls + 1;
        return (1 => (Value => 10), 2 => (Value => 20));
    end MakeRoots;
    package Source is
        function Make return Roots renames MakeRoots;
    end Source;
    type Wrapper is record
        Before : Integer;
        Items : Roots;
        After : Integer;
    end record;
    function MakeWrapper return Wrapper is
    begin
        Calls := Calls + 1;
        return (Before => 11, Items => (others => (Value => 30)), After => 99);
    end MakeWrapper;
    function AggregateComponent return Integer renames Roots'(others => (Value => 1))(1).Read;
    function TemporaryComponent return Integer renames MakeRoots (Index).Read;
    function QualifiedTemporary return Integer renames Source.Make (1).Read;
    function NestedTemporary return Integer renames MakeWrapper.Items (2).Read;
    generic
        with function Read return Integer;
    function GenericRead return Integer;
    function GenericRead return Integer is
    begin
        return Read;
    end GenericRead;
    function Instance is new GenericRead (MakeWrapper.Items (Index).Read);
    function DynamicSize return Integer is begin return 3; end DynamicSize;
    N : Integer := DynamicSize;
    type DynamicRoots is array (Integer range <>) of Prefixed_Model.Root;
    subtype SizedRoots is DynamicRoots (2 .. N + 2);
    function DynamicAggregate return Integer renames SizedRoots'(others => (Value => 40))(N).Read;
    function UnconstrainedAggregate return Integer renames DynamicRoots'(2 .. N => (Value => 50))(N).Read;
    function SlicedAggregate return Integer renames SizedRoots'(others => (Value => 55))(2 .. N)(N).Read;
    function MakeDynamic return DynamicRoots is
    begin
        Calls := Calls + 1;
        return (4 .. N + 4 => (Value => 60));
    end MakeDynamic;
    function DynamicTemporary return Integer renames MakeDynamic (N + 4).Read;
    type RuntimeWrapper is record
        Items : SizedRoots;
        Tail : Prefixed_Model.Root;
    end record;
    function RuntimeAggregate return Integer renames
        RuntimeWrapper'(Items => (others => (Value => 70)), Tail => (Value => 80)).Tail.Read;
    function MakeRuntime return RuntimeWrapper is
    begin
        Calls := Calls + 1;
        return (Items => (others => (Value => 90)), Tail => (Value => 100));
    end MakeRuntime;
    function RuntimeTemporary return Integer renames MakeRuntime.Items (N).Read;
    type Numbers is array (1 .. N) of Integer;
    type RuntimeRoot is tagged record
        Data : Numbers;
        Value : Integer;
    end record;
    function Read (Object : RuntimeRoot) return Integer is
    begin
        return Object.Data'Length + Object.Value;
    end Read;
    function DirectRuntime return Integer renames RuntimeRoot'(Data => (others => 2), Value => 120).Read;
    type Matrix is array (Integer range <>, Integer range <>) of Prefixed_Model.Root;
    subtype SizedMatrix is Matrix (1 .. N, 4 .. N + 4);
    function MatrixView return Integer renames SizedMatrix'(others => (others => (Value => 110)))(2, 5).Read;
    function ConvertedDynamic return Integer renames Prefixed_Model.Root(MakeDynamic (4)).Read;
    function ConvertedAggregate return Integer renames Prefixed_Model.Root(Prefixed_Model.Root'(Value => Index)).Read;
    -- Later declarations and calls exercise declaration temporary rewinds.
    Noise : Roots := MakeRoots;
begin
    N := 1;
    if Calls /= 8 or IndexCalls /= 3 then raise Program_Error; end if;
    for I in 1 .. 3 loop
        if AggregateComponent /= 1 or TemporaryComponent /= 20 or QualifiedTemporary /= 10
            or NestedTemporary /= 30 or Instance /= 30 or DynamicAggregate /= 40
            or UnconstrainedAggregate /= 50 or SlicedAggregate /= 55 or DynamicTemporary /= 60
            or ConvertedAggregate /= 2 or ConvertedDynamic /= 60 or RuntimeAggregate /= 80 or RuntimeTemporary /= 90
            or DirectRuntime /= 123 or MatrixView /= 110
            or Prefixed_Layout_Library.Read /= 130 or Prefixed_Layout_Library.Finalized /= 0 then
            raise Program_Error;
        end if;
    end loop;
    if Calls /= 8 or IndexCalls /= 3 then raise Program_Error; end if;
end PrefixedViewLayouts;
