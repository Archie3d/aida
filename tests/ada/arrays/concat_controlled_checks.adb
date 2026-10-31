with Ada.Unchecked_Deallocation;
with Ada.Finalization;
package body Concat_Controlled_Checks is
    procedure Run is
        Live : Integer := 0;
        type Guard is new Ada.Finalization.Controlled with record
            Number : Integer := 0;
            Fail : Boolean := False;
        end record;
        overriding procedure Initialize (X : in out Guard);
        overriding procedure Adjust (X : in out Guard);
        overriding procedure Finalize (X : in out Guard);
        type Link is access Guard;
        Pointer : Link := null;
        Mutate_Source : Boolean := False;
        procedure Free is new Ada.Unchecked_Deallocation (Guard, Link);
        overriding procedure Initialize (X : in out Guard) is
        begin
            Live := Live + 1;
        end Initialize;
        overriding procedure Adjust (X : in out Guard) is
        begin
            if X.Fail then
                raise Constraint_Error;
            end if;
            if Mutate_Source then
                Pointer.Number := 999;
            end if;
            Live := Live + 1;
        end Adjust;
        overriding procedure Finalize (X : in out Guard) is
        begin
            Live := Live - 1;
        end Finalize;
        type Guards is array (Positive range <>) of Guard;
        function Join (Left, Right : Guards) return Guards is
        begin
            return Left & Right;
        end Join;
        function Fail return Guards is
            Empty : Guards (1 .. 0);
        begin
            raise Constraint_Error;
            return Empty;
        end Fail;
        procedure Exercise is
            Source : Guards (5 .. 6);
            Other : Guards (9 .. 10);
            Before : Integer;
        begin
            Source (5).Number := 15;
            Source (6).Number := 16;
            Other (9).Number := 19;
            Other (10).Number := 20;
            declare
                Result : Guards := Join (Source, Other);
                Element_Left : Guards := Source (5) & Result;
                Element_Right : Guards := Result & Other (10);
                Pair : Guards := Source (5) & Other (9);
            begin
                if Result'First /= 5 or Result (8).Number /= 20
                    or Element_Left'First /= 1 or Element_Left (1).Number /= 15
                    or Element_Right (9).Number /= 20 or Pair (2).Number /= 19
                then
                    raise Program_Error;
                end if;
            end;
            if Live /= 4 then
                raise Program_Error;
            end if;
            Before := Live;
            Other (10).Fail := True;
            begin
                declare
                    Bad : Guards := Source & Other;
                begin
                    raise Constraint_Error;
                end;
            exception
                when Program_Error => null;
            end;
            if Live /= Before then
                raise Program_Error;
            end if;
            Other (10).Fail := False;
            begin
                declare
                    Bad : Guards := (Source & Source) & Fail;
                begin
                    raise Program_Error;
                end;
            exception
                when Constraint_Error => null;
            end;
            if Live /= Before then
                raise Program_Error;
            end if;
        end Exercise;
    begin
        Exercise;
        Pointer := new Guard;
        Pointer.Number := 123;
        declare
            Source : Guard;
        begin
            Mutate_Source := True;
            declare
                Snapshot : Guards := Source & Pointer.all;
            begin
                Mutate_Source := False;
                if Snapshot (2).Number /= 123 then
                    raise Program_Error;
                end if;
            end;
        end;
        Free (Pointer);
        if Live /= 0 then
            raise Program_Error;
        end if;
    end Run;
end Concat_Controlled_Checks;
