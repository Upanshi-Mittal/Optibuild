#!/bin/bash

OUTPUT_FILE="baseline_metrics.txt"

echo "Run at: $(date)" >> $OUTPUT_FILE
gtime -v ./app >> $OUTPUT_FILE 2>&1
echo "--------------------------" >> $OUTPUT_FILE
echo "\nRun completed. Metrics saved to $OUTPUT_FILE"