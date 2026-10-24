with Ada.Text_IO; use Ada.Text_IO;
procedure GotoStatements is
    Count : Integer := 0;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    package Local is
        procedure Run;
    end Local;
    package body Local is
        procedure Run is
        begin
            goto Finish;
            raise Program_Error;
            <<Finish>> null;
        end Run;
    begin
        goto Local.Ready;
        raise Program_Error;
        <<Ready>> null;
    end Local;
    procedure Nested is
    begin
        goto Finish;
        raise Program_Error;
        <<Finish>> null;
    end Nested;
begin
    goto Start;
    raise Program_Error;
    <<Start>> <<Again>>
    Count := Count + 1;
    if Count < 4 then
        goto aGaIn;
    end if;
    Check (Count = 4);
    -- A label on a compound statement targets its beginning.
    <<Restart>>
    for I in 1 .. 3 loop
        Count := Count + 1;
        if Count = 5 then
            goto Restart;
        end if;
        case I is
            when 2 => goto After_Loop;
            when others => null;
        end case;
    end loop;
    raise Program_Error;
    <<After_Loop>>
    Check (Count = 7);
    declare
        Finish : Integer := 0;
    begin
        goto GotoStatements.After_Shadow;
    end;
    raise Program_Error;
    <<After_Shadow>>
    Named_Block : begin
        if True then
            goto GotoStatements.Named_Block.Done;
        end if;
        raise Program_Error;
        <<Done>> null;
    end Named_Block;
    begin
        raise Constraint_Error;
    exception
        when Constraint_Error =>
            goto In_Handler;
            raise Program_Error;
            <<In_Handler>>
            goto GotoStatements.Finish;
    end;
    raise Program_Error;
    <<Finish>>
    Nested;
    Local.Run;
    Put_Line ("goto statements passed");
end GotoStatements;
