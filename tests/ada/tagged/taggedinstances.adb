with Ada.Text_IO;
with Ada.Unchecked_Deallocation;
procedure TaggedInstances is
    procedure Run is
        package Vehicles is
            type Vehicle is tagged limited private;
            type Link is access all Vehicle'Class;
            function Value (Self : Vehicle) return Integer;
            function Drive (Self : Vehicle) return Integer;
        private
            type Transmission (Owner : access Vehicle'Class) is limited null record;
            type Vehicle is tagged limited record
                Data : Integer := 4;
                Self : Link := Vehicle'Access;
                Power : Transmission (Vehicle'Access);
            end record;
        end Vehicles;
        package body Vehicles is
            function Value (Self : Vehicle) return Integer is
            begin
                return Self.Data;
            end Value;
            function Drive (Self : Vehicle) return Integer is
            begin
                return Value (Self.Self.all) + Value (Self.Power.Owner.all);
            end Drive;
        end Vehicles;
        package Cars is
            type Car is new Vehicles.Vehicle with record
                Extra : Integer := 9;
            end record;
            overriding function Value (Self : Car) return Integer;
        end Cars;
        package body Cars is
            overriding function Value (Self : Car) return Integer is
            begin
                return Self.Extra;
            end Value;
        end Cars;
        Object : Cars.Car;
        type Pointer is access Cars.Car;
        procedure Free is new Ada.Unchecked_Deallocation (Cars.Car, Pointer);
        procedure Allocate is
            Item : Pointer := new Cars.Car;
        begin
            if Cars.Drive (Item.all) /= 18 then
                raise Program_Error;
            end if;
            Free (Item);
        end Allocate;
        procedure Too_Deep is
            Item : Cars.Car;
        begin
            Ada.Text_IO.Put_Line ("deeper current instance wrongly accepted");
        end Too_Deep;
        Raised : Boolean := False;
    begin
        if Cars.Drive (Object) /= 18 then
            raise Program_Error;
        end if;
        Allocate;
        begin
            Too_Deep;
        exception
            when Program_Error => Raised := True;
        end;
        if not Raised then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("tagged instance worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("tagged current instances passed");
end TaggedInstances;
