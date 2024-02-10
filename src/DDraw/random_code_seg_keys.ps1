# Define the number of random numbers to generate
$numberOfRandomNumbers = 24

# Generate an array of random numbers
$randomNumbers = @()
for ($i = 0; $i -lt $numberOfRandomNumbers; $i++) {
    $randomNumbers += Get-Random -Minimum 0 -Maximum 4294967295
}

# Output the random numbers with the desired format
$output = ''
for ($i = 0; $i -lt $numberOfRandomNumbers; $i++) {
    $formattedNumber = "{0:X8}" -f $randomNumbers[$i]
    $output += "#define RANDOM_CODE_SEG_$i 0x$formattedNumber`n"
}

# Output the random numbers to a text file
$outputFilePath = "random_code_seg_keys.h"
$output | Out-File -FilePath $outputFilePath -Encoding ASCII
