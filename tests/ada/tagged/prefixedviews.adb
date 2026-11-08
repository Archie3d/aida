with Prefixed_Model;
with Prefixed_View_Library;
procedure PrefixedViews is
    X : aliased Prefixed_Model.Root;
    Y : aliased Prefixed_Model.Root;
    Value : aliased Prefixed_Model.Root;
    type Pointer is access all Prefixed_Model.Root;
    Current : Pointer := X'Access;
    Calls : Integer := 0;
    function Pick return Pointer is
    begin
        Calls := Calls + 1;
        return Current;
    end Pick;
    function Read return Integer renames Pick.Read;
    procedure Add (Amount : Integer := 3) renames X.Add;
    procedure Set (Value : Integer) renames X.Set;
    procedure SetValue (Amount : Integer) renames Value.Set;
    function Copy return Prefixed_Model.Root renames X.Copy;
    function ViaAccess return Integer renames X.Via_Access;
    generic
        with function Getter return Integer;
        with procedure Setter (Value : Integer);
    procedure Exercise;
    procedure Exercise is
    begin
        Setter (Getter + 7);
    end Exercise;
    procedure Instance is new Exercise (Pick.Read, X.Set);
    procedure OtherInstance is new Exercise (Value.Read, Value.Set);
    C : aliased Prefixed_Model.Root'Class := Prefixed_Model.Child'(Value => 4, Extra => 20);
    function Dispatch return Integer renames C.Read;
    function DispatchAccess return Integer renames C.Via_Access;
    procedure Completed (Amount : Integer);
    procedure Completed (Amount : Integer) renames X.Add;
begin
    if Prefixed_View_Library.Read /= 73 or Prefixed_View_Library.LiveCount /= 1 then
        raise Program_Error;
    end if;
    if Calls /= 2 then raise Program_Error; end if;
    Current := Y'Access;
    Set (11);
    SetValue (17);
    OtherInstance;
    if Value.Value /= 24 then raise Program_Error; end if;
    Add;
    Completed (1);
    if Read /= 15 or ViaAccess /= 15 then raise Program_Error; end if;
    Instance;
    if Read /= 22 or X.Value /= 22 or Y.Value /= 1 or Calls /= 2 then
        raise Program_Error;
    end if;
    if Copy.Value /= 22 then raise Program_Error; end if;
    if Dispatch /= 24 or DispatchAccess /= 24 then raise Program_Error; end if;
    Current := null;
    begin
        declare
            function Invalid return Integer renames Pick.Read;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    begin
        declare
            procedure Invalid is new Exercise (Pick.Read, X.Set);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    if Calls /= 4 or Read /= 22 then raise Program_Error; end if;
end PrefixedViews;
