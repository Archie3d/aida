with Ada.Finalization;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure ControlledVariants is
    Live, Other_Live, Starts, Adjusts : Integer := 0;
    Fail_Init, Fail_Adjust : Integer := 0;
    Fail_Finalize : Boolean := False;
    Checks : Integer := 0;
    procedure Check (Condition : Boolean) is
    begin
        Checks := Checks + 1;
        if not Condition then
            Put_Line ("failed check" & Integer'Image (Checks));
            Put_Line ("live" & Integer'Image (Live) & " other" & Integer'Image (Other_Live));
            raise Program_Error;
        end if;
    end Check;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 17;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Starts := Starts + 1;
        if Starts = Fail_Init then
            raise Constraint_Error;
        end if;
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (Object : in out Guard) is
    begin
        Adjusts := Adjusts + 1;
        if Adjusts = Fail_Adjust then
            raise Constraint_Error;
        end if;
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Check (Object.Value = 17);
        Live := Live - 1;
        if Fail_Finalize then
            raise Constraint_Error;
        end if;
    end Finalize;
    type Other_Guard is new Ada.Finalization.Controlled with null record;
    overriding procedure Initialize (Object : in out Other_Guard);
    overriding procedure Adjust (Object : in out Other_Guard);
    overriding procedure Finalize (Object : in out Other_Guard);
    overriding procedure Initialize (Object : in out Other_Guard) is
    begin
        Other_Live := Other_Live + 1;
    end Initialize;
    overriding procedure Adjust (Object : in out Other_Guard) is
    begin
        Other_Live := Other_Live + 1;
    end Adjust;
    overriding procedure Finalize (Object : in out Other_Guard) is
    begin
        Other_Live := Other_Live - 1;
    end Finalize;
    type Guards is array (1 .. 2) of Guard;
    type Item (Kind : Integer := 0; Inner : Boolean := False) is record
        case Kind is
            when 1 => One : Guard;
            when 2 => Other : Other_Guard;
            when 3 =>
                case Inner is
                    when True => Many : Guards;
                    when False => null;
                end case;
            when others => Number : Integer := 42;
        end case;
    end record;
    type Envelope is tagged record
        Payload : Item;
    end record;
    type Buffer (Present : Boolean := True; Length : Natural := 2) is record
        Text : String (1 .. Length) := (others => 'x');
        case Present is
            when True => Value : Guard;
            when False => null;
        end case;
    end record;
    type Link is access Item;
    procedure Free is new Ada.Unchecked_Deallocation (Item, Link);
    procedure Replace (Target : in out Item; Source : Item) is
    begin
        Target := Source;
    end Replace;
    function Make return Item is
        Result : Item (3, True);
    begin
        return Result;
    end Make;
begin
    declare
        Empty : Item;
        A : Item (1, False);
        B : Item (2, False);
        C : Item (3, True);
        Inactive : Item (3, False);
        Copy : Item := C;
        Heap : Link := new Item'(A);
    begin
        Check (Live = 6 and Other_Live = 1);
        Replace (Empty, A);
        Check (Live = 7 and Empty.Kind = 1);
        Replace (Empty, B);
        Check (Live = 6 and Other_Live = 2 and Empty.Kind = 2);
        Replace (Empty, C);
        Check (Live = 8 and Other_Live = 1 and Empty.Many (2).Value = 17);
        Replace (Empty, Inactive);
        Check (Live = 6 and Empty.Kind = 3 and not Empty.Inner);
        Copy := Copy;
        begin
            Replace (A, B);
            raise Program_Error;
        exception
            when Constraint_Error => Check (Live = 6 and Other_Live = 1);
        end;
        Free (Heap);
        Check (Live = 5 and Heap = null);
        Heap := new Item'(Make);
        Check (Live = 7);
        Free (Heap);
    end;
    Check (Live = 0 and Other_Live = 0);
    Put_Line ("active variants, copies, assignment and allocation");
    declare
        A : Envelope;
        B : Envelope'Class := A;
        Source : Item (3, True);
    begin
        A.Payload := Source;
        B := Envelope'Class (A);
        Check (Live = 6);
        A.Payload := (2, False, Other => <>);
        B := Envelope'Class (A);
        Check (Live = 2 and Other_Live = 2);
    end;
    Check (Live = 0 and Other_Live = 0);
    declare
        A : Buffer;
        B : Buffer := (False, 4, Text => "test");
    begin
        Check (Live = 1);
        A := B;
        Check (Live = 0 and A.Text = "test");
        B := (True, 3, Text => "new", Value => <>);
        A := B;
        Check (Live = 2 and A.Text = "new");
    end;
    Check (Live = 0);
    Put_Line ("class-wide copies and dependent storage");
    Fail_Init := Starts + 2;
    begin
        declare
            Broken : Item (3, True);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => Check (Live = 0);
    end;
    Fail_Init := 0;
    declare
        Source : Item (3, True);
    begin
        Fail_Adjust := Adjusts + 1;
        begin
            declare
                Broken : Item := Source;
            begin
                raise Constraint_Error;
            end;
        exception
            when Program_Error => Check (Live = 2);
        end;
        Fail_Adjust := 0;
    end;
    Check (Live = 0);
    declare
        Source : Item (3, True);
        Target : Item;
    begin
        -- Disjoint assignment adjusts the target directly.
        Fail_Adjust := Adjusts + 1;
        begin
            Target := Source;
            raise Constraint_Error;
        exception
            when Program_Error => Check (Live = 3 and Target.Kind = 3);
        end;
        Fail_Adjust := 0;
        Target := (2, False, Other => <>);
        Check (Live = 2 and Other_Live = 1);
        Target := Source;
        Check (Live = 4 and Other_Live = 0);
    end;
    Check (Live = 0 and Other_Live = 0);
    Put_Line ("partial initialization and adjustment");
    declare
        Source : Item (2, False);
        Target : Item := (3, True, Many => <>);
    begin
        Fail_Finalize := True;
        begin
            Target := Source;
            raise Constraint_Error;
        exception
            when Program_Error => Check (Live = 0 and Other_Live = 1);
        end;
        Fail_Finalize := False;
        Target := Source;
        Check (Other_Live = 2);
    end;
    Check (Live = 0 and Other_Live = 0);
    Put_Line ("finalization failure and recovery");
    declare
        type Items is array (1 .. 3) of Item;
        Values : Items;
        Source : Item (1, False);
    begin
        Values (1) := Source;
        Values (3) := Source;
        Check (Live = 3);
    end;
    Check (Live = 0);
    Put_Line ("controlled variants passed");
end ControlledVariants;
