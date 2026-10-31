with Ada.Finalization;
with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure RuntimeComponentCleanup is
    N : Integer := 2;
    Live : Integer := 0;
    Starts : Integer := 0;
    Fail_At : Integer := 0;
    type Guard is new Ada.Finalization.Controlled with record
        Active : Boolean := False;
    end record;
    overriding procedure Initialize (X : in out Guard);
    overriding procedure Adjust (X : in out Guard);
    overriding procedure Finalize (X : in out Guard);
    overriding procedure Initialize (X : in out Guard) is
    begin
        Starts := Starts + 1;
        if Starts = Fail_At then raise Constraint_Error; end if;
        X.Active := True;
        Live := Live + 1;
    end Initialize;
    overriding procedure Adjust (X : in out Guard) is
    begin
        Live := Live + 1;
    end Adjust;
    overriding procedure Finalize (X : in out Guard) is
    begin
        if X.Active then Live := Live - 1; end if;
    end Finalize;
    type Guards is array (Integer range <>) of Guard;
    subtype Two is Guards (1 .. N);
    type Holder is record
        Text : String (1 .. N) := (others => 'x');
        Data : Two;
        Last : Integer := 42;
    end record;
    type Pointer is access Guards;
    type Holder_Pointer is access Holder;
    procedure Free is new Ada.Unchecked_Deallocation (Guards, Pointer);
    procedure Free_Holder is new Ada.Unchecked_Deallocation (Holder, Holder_Pointer);
    type Limited_Guard is new Ada.Finalization.Limited_Controlled with record
        Value : Integer := 7;
    end record;
    Finalized : Integer := 0;
    overriding procedure Finalize (X : in out Limited_Guard) is
    begin
        if X.Value /= 7 then raise Program_Error; end if;
        Finalized := Finalized + 1;
    end Finalize;
    type Limited_Array is array (Integer range <>) of Limited_Guard;
    type Limited_Pointer is access Limited_Array;
    procedure Free_Limited is new Ada.Unchecked_Deallocation (Limited_Array, Limited_Pointer);
    function Make return Limited_Array is
    begin
        return (3 .. N + 2 => (Ada.Finalization.Limited_Controlled with Value => 7));
    end Make;
begin
    declare
        A : Holder;
        B : Holder := A;
        P : Pointer := new Guards (4 .. N + 3);
        H : Holder_Pointer := new Holder;
        L : Limited_Pointer := new Limited_Array'(Make);
        G : Limited_Pointer := new Limited_Array'
            (5 .. N + 4 => (Ada.Finalization.Limited_Controlled with Value => 7));
    begin
        if Live /= 8 or P.all'First /= 4 or H.Last /= 42 or L.all'First /= 3
            or G.all'First /= 5 then raise Program_Error; end if;
        B := A;
        if Live /= 8 then raise Program_Error; end if;
        Free (P); Free_Holder (H); Free_Limited (L); Free_Limited (G);
        if Live /= 4 or Finalized /= 4 then raise Program_Error; end if;
    end;
    if Live /= 0 then raise Program_Error; end if;
    Fail_At := Starts + 2;
    begin
        declare
            P : Pointer := new Guards (1 .. N + 1);
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    if Live /= 0 then raise Program_Error; end if;
    Fail_At := Starts + 2;
    begin
        declare
            P : Holder_Pointer := new Holder;
        begin
            raise Program_Error;
        end;
    exception
        when Constraint_Error => null;
    end;
    if Live /= 0 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("runtime component cleanup ok");
end RuntimeComponentCleanup;
