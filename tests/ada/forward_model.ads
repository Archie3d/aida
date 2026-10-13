package Forward_Model is
    generic
        Value : Integer;
    function Template return Integer;
    function Invoke (Value : Integer) return Integer;
    Early : Integer;
end Forward_Model;
