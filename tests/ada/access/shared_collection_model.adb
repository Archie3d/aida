package body Shared_Collection_Model is
    protected body Counts is
        procedure Add (Amount : Integer) is
        begin
            Objects := Objects + Amount;
        end Add;
        procedure Completed is
        begin
            Finished := Finished + 1;
        end Completed;
        procedure Failed is
        begin
            Failures := Failures + 1;
        end Failed;
        function Live return Integer is
        begin
            return Objects;
        end Live;
        function Done return Integer is
        begin
            return Finished;
        end Done;
        function Errors return Integer is
        begin
            return Failures;
        end Errors;
    end Counts;
    overriding procedure Initialize (Object : in out Guard) is
    begin
        Counts.Add (1);
    end Initialize;
    overriding procedure Adjust (Object : in out Guard) is
    begin
        Counts.Add (1);
    end Adjust;
    overriding procedure Finalize (Object : in out Guard) is
    begin
        Counts.Add (-1);
    end Finalize;
end Shared_Collection_Model;
