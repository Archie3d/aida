with Ada.Text_IO; use Ada.Text_IO;
procedure ConcurrentDiscriminants is
    task type Worker (Id : Integer; P : access Integer) is entry Go; end Worker;
    task body Worker is
        function Value return Integer is begin return Id; end Value;
    begin
        accept Go do P.all := Value; end Go;
    end Worker;
    protected type Gate (Size : Positive := 3) is
        function Get return Integer;
        procedure Set;
    private
        Text : String (1 .. Size) := (others => 'a');
    end Gate;
    protected body Gate is
        function Get return Integer is begin return Text'Length + Size; end Get;
        procedure Set is begin Text (Size) := 'b'; end Set;
    end Gate;
    X : aliased Integer := 0;
    A : Worker (7, X'Access);
    B : Gate (4);
begin
    A.Go;
    B.Set;
    if X /= 7 or B.Get /= 8 or B.Size /= 4 or A.Id /= 7 then raise Program_Error; end if;
    declare
        task type Server (Count : Positive) is
            entry Work (1 .. Count) (N : in out Integer);
        end Server;
        task body Server is
            function Last return Integer is begin return Server.Count; end Last;
        begin
            accept Work (Last) (N : in out Integer) do N := N + Count; end Work;
        end Server;
        Object : Server (4);
        Index : Integer := 4;
        procedure Send (V : in out Integer) renames Object.Work (Index);
        Value : Integer := 0;
    begin
        Index := 1;
        Send (Value);
        if Value /= 4 then raise Program_Error; end if;
    end;
    declare
        Calls : Natural := 0;
        function Next return Positive is
        begin Calls := Calls + 1; return Calls; end Next;
        protected type Defaulted (N : Positive := Next; P : access Integer := null) is
            function Value return Integer;
        end Defaulted;
        protected body Defaulted is
            function Value return Integer is
            begin
                if P /= null then return P.all; end if;
                return Defaulted.N;
            end Value;
        end Defaulted;
        One, Two : Defaulted;
        Three : Defaulted (9, new Integer'(12));
    begin
        if Calls /= 2 or One.Value /= 1 or Two.Value /= 2 or Three.Value /= 12 then raise Program_Error; end if;
    end;
    -- Finalize a protected destination's queues before freeing its private
    -- discriminant-dependent storage, even when a task redirects a live call.
    declare
        task Creator is entry Work; end Creator;
        task body Creator is
            protected type Local_Type (N : Positive := 8) is entry Wait;
            private Text : String (1 .. N) := (others => 'a'); end Local_Type;
            protected body Local_Type is
                entry Wait when False is begin raise Constraint_Error; end Wait;
            end Local_Type;
            Local : Local_Type;
        begin accept Work do requeue Local.Wait; end Work; end Creator;
    begin
        begin Creator.Work; raise Constraint_Error;
        exception when Program_Error => null;
        end;
    end;
    begin
        declare
            function Fail return Integer is begin raise Constraint_Error; return 0; end Fail;
            protected type Broken (N : Positive) is procedure Noop;
            private
                Text : String (1 .. N) := (others => 'a');
                Invalid : Integer := Fail;
            end Broken;
            protected body Broken is procedure Noop is begin null; end Noop; end Broken;
            Object : Broken (64);
        begin raise Program_Error;
        end;
    exception when Constraint_Error => null;
    end;
    Put_Line ("concurrent discriminants OK");
end ConcurrentDiscriminants;
