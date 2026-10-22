with Ada.Finalization;
with Ada.Unchecked_Deallocation;
package Shared_Collection_Model is
    protected Counts is
        procedure Add (Amount : Integer);
        procedure Completed;
        procedure Failed;
        function Live return Integer;
        function Done return Integer;
        function Errors return Integer;
    private
        Objects : Integer := 0;
        Finished : Integer := 0;
        Failures : Integer := 0;
    end Counts;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 0;
    end record;
    overriding procedure Initialize (Object : in out Guard);
    overriding procedure Adjust (Object : in out Guard);
    overriding procedure Finalize (Object : in out Guard);
    type Link is access Guard;
    type Derived_Link is new Link;
    procedure Free is new Ada.Unchecked_Deallocation (Guard, Link);
    procedure Free_Derived is new Ada.Unchecked_Deallocation (Guard, Derived_Link);
end Shared_Collection_Model;
