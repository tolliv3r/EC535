#!/bin/bash

echo "=== ULTRA-PRECISE PERFORMANCE TESTING ==="
echo

# Test files
TEST_FILES=("sample.assembly" "large_test.assembly")

for test_file in "${TEST_FILES[@]}"; do
    if [ -f "$test_file" ]; then
        echo "Testing with $test_file:"
        echo "========================================"
        
        # Test each version multiple times for accuracy
        versions=("myISS" "myISS_optimized" "myISS_final")
        
        for version in "${versions[@]}"; do
            if [ -f "$version" ]; then
                echo "Testing $version:"
                
                # Run 50 iterations and calculate average
                total_time=0
                for i in {1..50}; do
                    start_time=$(date +%s.%N)
                    ./$version "$test_file" > /dev/null 2>&1
                    end_time=$(date +%s.%N)
                    runtime=$(echo "$end_time - $start_time" | bc -l)
                    total_time=$(echo "$total_time + $runtime" | bc -l)
                done
                
                avg_time=$(echo "scale=6; $total_time / 50" | bc -l)
                echo "  Average runtime over 50 runs: $avg_time seconds"
                echo
            fi
        done
        echo
    fi
done

echo "=== LEADERBOARD COMPARISON ==="
echo "Target times from leaderboard:"
echo "  Bennett: 0.004965 seconds"
echo "  covalent: 0.005428 seconds"
echo "  nsearls: 0.011573 seconds"
echo "  Noah H: 0.011949 seconds"
echo "  qqq: 0.01196 seconds"
echo
echo "Your current best time should be around 0.005-0.012 seconds to compete!"
